import re
import json
import math
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


# =========================
# CONFIG
# =========================
MOTE_OUTPUT_PATH = Path("moteOutput.txt")
RADIO_MSG_PATH = Path("dadosRadioMessage.txt")  # seu export do Radio Messages
OUT_DIR = Path("cooja_metrics_out")
OUT_DIR.mkdir(exist_ok=True)

# Posição da base no cenário
BASE_XYZ = (0.0, 0.0, 0.0)

# IDs dos motes (ajuste se necessário)
ROCKET_ID = 1
BASE_ID = 2

# Binning de distância (metros se você manda p em metros)
DIST_BIN_SIZE = 250

# Preferir Radio Messages para “confirmar” RX (recomendado)
USE_RADIO_MESSAGES_FOR_RX = True


# =========================
# HELPERS
# =========================
def parse_sim_ts_mmss(ts: str) -> float:
    """Converte 'MM:SS.mmm' para segundos float."""
    mm, rest = ts.split(":")
    ss, ms = rest.split(".")
    return int(mm) * 60 + int(ss) + int(ms) / 1000.0


def dist_xyz(x, y, z, base_xyz=BASE_XYZ) -> float:
    if x is None or y is None or z is None:
        return float("nan")
    bx, by, bz = base_xyz
    return math.sqrt((x - bx) ** 2 + (y - by) ** 2 + (z - bz) ** 2)


def safe_json_loads(s: str):
    try:
        return json.loads(s)
    except Exception:
        return None


def decode_hex_payload_to_json(hex_blob: str):
    """
    Recebe blob hex com espaços (ex: 'ABFFFF00 01005A81 ...'),
    converte para bytes, acha '{...}' e tenta json.loads.
    Retorna (payload_dict, payload_str) ou (None, None).
    """
    hx = re.sub(r"[^0-9A-Fa-f]", "", hex_blob)
    if len(hx) < 2:
        return None, None
    if len(hx) % 2 == 1:
        hx = hx[:-1]

    try:
        b = bytes.fromhex(hx)
    except Exception:
        return None, None

    i = b.find(b"{")
    j = b.rfind(b"}")
    if i == -1 or j == -1 or j <= i:
        return None, None

    raw = b[i:j + 1]
    s = raw.decode("utf-8", errors="ignore")
    payload = safe_json_loads(s)
    if isinstance(payload, dict):
        return payload, s
    return None, None


def parse_radio_time_to_seconds(t0: str) -> float:
    """
    No Radio Messages, a 1ª coluna costuma ser contador em ms.
    Ex: 29667 -> 29.667s
    Se o seu arquivo estiver em outra unidade, ajuste aqui.
    """
    try:
        v = float(t0)
    except Exception:
        return float("nan")

    if v > 1000:
        return v / 1000.0
    return v


def compute_blackout_ranges(missing_seqs):
    """Agrupa sequências faltantes em ranges contínuos (start,end)."""
    if not missing_seqs:
        return []
    missing = sorted(missing_seqs)
    out = []
    start = prev = missing[0]
    for s in missing[1:]:
        if s == prev + 1:
            prev = s
        else:
            out.append((start, prev))
            start = prev = s
    out.append((start, prev))
    return out


def to_py(v):
    """Converte tipos numpy/pandas para tipos nativos do Python (JSON-safe)."""
    if v is None:
        return None
    if isinstance(v, (np.integer,)):
        return int(v)
    if isinstance(v, (np.floating,)):
        return float(v)
    if isinstance(v, (np.bool_,)):
        return bool(v)
    if isinstance(v, np.ndarray):
        return v.tolist()
    return v


def dict_to_py(d):
    """Recursivo: converte dict/list com numpy/pandas para JSON-safe."""
    if isinstance(d, dict):
        return {k: dict_to_py(v) for k, v in d.items()}
    if isinstance(d, list):
        return [dict_to_py(x) for x in d]
    return to_py(d)


# =========================
# REGEX (moteOutput)
# =========================
LINE_RE = re.compile(r"^(?P<ts>\d{2}:\d{2}\.\d{3})\s+ID:(?P<id>\d+)\s+(?P<msg>.*)$")
TX_RE = re.compile(
    r"\[Rocket\]\s+t=(?P<tsec>\d+)s\s+TX\s+SF(?P<sf>\d+)\s+airtime=(?P<air>\d+)ms\s+bytes=(?P<bytes>\d+)\s+payload=(?P<payload>\{.*\})"
)
RX_RE = re.compile(
    r"\[Base\]\s+Recebido de\s+(?P<src>\d+\.\d+)\s+->\s+\"(?P<payload>\{.*\})\"\s+\(tamanho=(?P<bytes>\d+)\s+bytes\)"
)


# =========================
# PARSE moteOutput (TX + RX “lógico”)
# =========================
tx_rows = []
rx_rows_mote = []

if MOTE_OUTPUT_PATH.exists():
    mote_lines = MOTE_OUTPUT_PATH.read_text(errors="ignore").splitlines()

    for line in mote_lines:
        m = LINE_RE.match(line)
        if not m:
            continue

        sim_time_s = parse_sim_ts_mmss(m.group("ts"))
        msg = m.group("msg")

        mtx = TX_RE.search(msg)
        if mtx:
            payload_str = mtx.group("payload")
            payload = safe_json_loads(payload_str)
            if not isinstance(payload, dict):
                continue

            seq = payload.get("s")
            p = payload.get("p", [None, None, None])
            if not isinstance(p, list):
                p = [None, None, None]
            x, y, z = (p + [None, None, None])[:3]

            tx_rows.append({
                "sim_time_s": sim_time_s,
                "flight_time_s": int(mtx.group("tsec")),
                "sf": int(mtx.group("sf")),
                "airtime_ms": int(mtx.group("air")),
                "tx_bytes": int(mtx.group("bytes")),
                "seq": seq,
                "x": x, "y": y, "z": z,
                "payload_json": payload_str
            })
            continue

        mrx = RX_RE.search(msg)
        if mrx:
            payload_str = mrx.group("payload")
            payload = safe_json_loads(payload_str)
            if not isinstance(payload, dict):
                continue

            seq = payload.get("s")
            p = payload.get("p", [None, None, None])
            if not isinstance(p, list):
                p = [None, None, None]
            x, y, z = (p + [None, None, None])[:3]

            rx_rows_mote.append({
                "sim_time_s": sim_time_s,
                "src": mrx.group("src"),
                "rx_bytes": int(mrx.group("bytes")),
                "seq": seq,
                "x": x, "y": y, "z": z,
                "payload_json": payload_str
            })

tx_df = pd.DataFrame(tx_rows)
rx_mote_df = pd.DataFrame(rx_rows_mote)

if len(tx_df):
    tx_df = tx_df.dropna(subset=["seq"]).sort_values("seq")
    tx_df["seq"] = tx_df["seq"].astype(int)
    tx_df["dist"] = tx_df.apply(lambda r: dist_xyz(r["x"], r["y"], r["z"]), axis=1)

if len(rx_mote_df):
    rx_mote_df = rx_mote_df.dropna(subset=["seq"]).sort_values("seq")
    rx_mote_df["seq"] = rx_mote_df["seq"].astype(int)
    rx_mote_df["dist"] = rx_mote_df.apply(lambda r: dist_xyz(r["x"], r["y"], r["z"]), axis=1)


# =========================
# PARSE Radio Messages (RX “físico”)
# =========================
radio_rows = []
rx_rows_radio = []

if RADIO_MSG_PATH.exists():
    for line in RADIO_MSG_PATH.read_text(errors="ignore").splitlines():
        line = line.strip()
        if not line:
            continue

        # Exemplo esperado:
        # time  src  dst_or_-  len: ... HEX...
        parts = re.split(r"\s+", line)
        if len(parts) < 6:
            continue

        t0, src, dst = parts[0], parts[1], parts[2]

        # encontra início do hex no final da linha
        hex_start_idx = None
        for i in range(3, len(parts)):
            if re.fullmatch(r"[0-9A-Fa-f]{8,}", parts[i]) is not None:
                hex_start_idx = i
                break
            if re.fullmatch(r"[0-9A-Fa-f]+", parts[i] or "") and any(c in "ABCDEFabcdef" for c in parts[i]):
                hex_start_idx = i
                break

        if hex_start_idx is None:
            continue

        hex_blob = " ".join(parts[hex_start_idx:])
        payload, payload_str = decode_hex_payload_to_json(hex_blob)
        if not isinstance(payload, dict):
            continue

        seq = payload.get("s")
        if seq is None:
            continue

        p = payload.get("p", [None, None, None])
        if not isinstance(p, list):
            p = [None, None, None]
        x, y, z = (p + [None, None, None])[:3]

        sim_time_s = parse_radio_time_to_seconds(t0)

        radio_rows.append({
            "sim_time_s": sim_time_s,
            "src": src,
            "dst": dst,
            "seq": int(seq),
            "x": x, "y": y, "z": z,
            "payload_json": payload_str
        })

        # considera RX quando dst == BASE_ID
        if dst.isdigit() and int(dst) == BASE_ID:
            rx_rows_radio.append({
                "sim_time_s": sim_time_s,
                "src": src,
                "seq": int(seq),
                "rx_bytes": len(payload_str.encode("utf-8")),
                "x": x, "y": y, "z": z,
                "payload_json": payload_str
            })

radio_df = pd.DataFrame(radio_rows)
rx_radio_df = pd.DataFrame(rx_rows_radio)

if len(rx_radio_df):
    rx_radio_df = rx_radio_df.sort_values("seq")
    rx_radio_df["dist"] = rx_radio_df.apply(lambda r: dist_xyz(r["x"], r["y"], r["z"]), axis=1)


# =========================
# Decide fonte de RX
# =========================
rx_df = rx_radio_df if (USE_RADIO_MESSAGES_FOR_RX and len(rx_radio_df)) else rx_mote_df

# =========================
# MERGE TX + RX por seq
# =========================
if not len(tx_df):
    raise SystemExit("Não achei TX no moteOutput.txt. Verifique se ele contém as linhas [Rocket] t=... TX ...")

rx_merge = (
    rx_df[["seq", "sim_time_s"]].rename(columns={"sim_time_s": "rx_time_s"})
    if len(rx_df) else pd.DataFrame(columns=["seq", "rx_time_s"])
)

merged = tx_df.merge(rx_merge, on="seq", how="left")
merged["delivered"] = ~merged["rx_time_s"].isna()


# =========================
# METRICS
# =========================
tx_count = len(tx_df)
rx_count = int(merged["delivered"].sum())

pdr = (rx_count / tx_count) if tx_count else float("nan")
per = (1 - pdr) if tx_count else float("nan")

# última posição com comunicação (último seq entregue)
deliv = merged[merged["delivered"]].sort_values("seq")
last_pos = None
if len(deliv):
    last = deliv.iloc[-1]
    last_pos = {
        "seq": int(last["seq"]),
        "sim_time_s": float(last["rx_time_s"]),
        "x": float(last["x"]) if last["x"] is not None and not pd.isna(last["x"]) else None,
        "y": float(last["y"]) if last["y"] is not None and not pd.isna(last["y"]) else None,
        "z": float(last["z"]) if last["z"] is not None and not pd.isna(last["z"]) else None,
        "dist": float(last["dist"]) if not pd.isna(last["dist"]) else None,
    }

# intervalos e jitter
rx_times = deliv["rx_time_s"].to_numpy() if len(deliv) else np.array([])
rx_intervals = np.diff(rx_times) if len(rx_times) > 1 else np.array([])
mean_rx_interval_s = float(np.mean(rx_intervals)) if len(rx_intervals) else float("nan")
jitter_std_s = float(np.std(rx_intervals, ddof=1)) if len(rx_intervals) > 1 else 0.0

# throughput efetivo
throughput_Bps = 0.0
if len(deliv) >= 2:
    duration = rx_times[-1] - rx_times[0]
    if "rx_bytes" in rx_df.columns and len(rx_df):
        rb = rx_df.drop_duplicates("seq").set_index("seq")["rx_bytes"]
        bytes_rx = float(rb.reindex(deliv["seq"]).fillna(0).sum())
    else:
        bytes_rx = float(deliv["tx_bytes"].sum())
    throughput_Bps = (bytes_rx / duration) if duration > 0 else 0.0

# blackouts por sequência
tx_seqs = set(tx_df["seq"].tolist())
rx_seqs = set(deliv["seq"].tolist())
missing = sorted(list(tx_seqs - rx_seqs))
blackouts = compute_blackout_ranges(missing)

# PDR x distância (binned)
max_d = float(np.nanmax(tx_df["dist"])) if tx_count else 0.0
bins = np.arange(0, max_d + DIST_BIN_SIZE, DIST_BIN_SIZE) if max_d > 0 else np.array([0, DIST_BIN_SIZE])

tx_df["dist_bin"] = pd.cut(tx_df["dist"], bins=bins, include_lowest=True)

deliv_tx = tx_df[tx_df["seq"].isin(rx_seqs)].copy()
deliv_tx["dist_bin"] = pd.cut(deliv_tx["dist"], bins=bins, include_lowest=True)

# observed=False explícito para evitar FutureWarning
pdr_by_bin = (
    deliv_tx.groupby("dist_bin", observed=False)["seq"].count()
    / tx_df.groupby("dist_bin", observed=False)["seq"].count()
).fillna(0).reset_index()
pdr_by_bin.columns = ["dist_bin", "pdr"]


# =========================
# SAVE FILES
# =========================
merged.to_csv(OUT_DIR / "packets_merged.csv", index=False)
pdr_by_bin.to_csv(OUT_DIR / "pdr_by_distance_bin.csv", index=False)

summary = {
    "tx_packets": int(tx_count),
    "rx_packets": int(rx_count),
    "PDR": float(pdr) if not pd.isna(pdr) else None,
    "PER": float(per) if not pd.isna(per) else None,
    "mean_rx_interval_s": float(mean_rx_interval_s) if not pd.isna(mean_rx_interval_s) else None,
    "jitter_std_s": float(jitter_std_s) if not pd.isna(jitter_std_s) else None,
    "throughput_bytes_per_s": float(throughput_Bps),
    "last_position_with_comm": last_pos,
    "blackout_seq_ranges": [[int(a), int(b)] for (a, b) in blackouts],
    "rx_source": "radio_messages" if (USE_RADIO_MESSAGES_FOR_RX and len(rx_radio_df)) else "moteOutput",
}

summary = dict_to_py(summary)

(OUT_DIR / "summary.json").write_text(
    json.dumps(summary, indent=2, ensure_ascii=False),
    encoding="utf-8"
)


# =========================
# PLOTS
# =========================
# 1) PDR vs distance
plt.figure()
bin_mid = []
for interval in pdr_by_bin["dist_bin"]:
    if pd.isna(interval):
        bin_mid.append(np.nan)
    else:
        bin_mid.append((interval.left + interval.right) / 2)

plt.plot(bin_mid, pdr_by_bin["pdr"].to_numpy())
plt.xlabel("Distance (m)")
plt.ylabel("PDR")
plt.title("PDR vs Distance (binned)")
plt.grid(True)
plt.tight_layout()
plt.savefig(OUT_DIR / "pdr_vs_distance.png", dpi=200)
plt.close()

# 2) RX inter-arrival
plt.figure()
if len(rx_intervals) > 0:
    plt.plot(rx_intervals)
plt.xlabel("RX packet index")
plt.ylabel("Inter-arrival time (s)")
plt.title("RX Inter-arrival Times")
plt.grid(True)
plt.tight_layout()
plt.savefig(OUT_DIR / "rx_intervals.png", dpi=200)
plt.close()

# 3) Blackouts timeline: 1=ok, 0=lost + spans em blackout
plt.figure(figsize=(10, 3))
seqs = tx_df["seq"].to_numpy()
delivered_flag = merged["delivered"].astype(int).to_numpy()

plt.plot(seqs, delivered_flag)
plt.ylim(-0.1, 1.1)
plt.yticks([0, 1], ["lost", "ok"])
plt.xlabel("Sequence")
plt.ylabel("Delivery")
plt.title("Blackouts (lost ranges highlighted)")

for a, b in blackouts:
    plt.axvspan(a, b, alpha=0.25)

plt.grid(True, axis="x")
plt.tight_layout()
plt.savefig(OUT_DIR / "blackouts_timeline.png", dpi=200)
plt.close()

# 4) Blackout ranges (Gantt simples)
plt.figure(figsize=(10, 3))
if blackouts:
    y = 0
    for (a, b) in blackouts:
        plt.hlines(y, a, b, linewidth=6)
        y += 1
    plt.yticks([])
    plt.xlabel("Sequence")
    plt.title("Blackout ranges (by sequence)")
    plt.grid(True, axis="x")
else:
    plt.text(0.5, 0.5, "No blackouts detected", ha="center", va="center")
    plt.axis("off")

plt.tight_layout()
plt.savefig(OUT_DIR / "blackouts_ranges.png", dpi=200)
plt.close()

print("OK! Outputs saved to:", OUT_DIR.resolve())
print("Summary:", json.dumps(summary, indent=2, ensure_ascii=False)[:1400])
print("Plots:",
      (OUT_DIR / "pdr_vs_distance.png").resolve(),
      (OUT_DIR / "rx_intervals.png").resolve(),
      (OUT_DIR / "blackouts_timeline.png").resolve(),
      (OUT_DIR / "blackouts_ranges.png").resolve(),
      sep="\n  - ")
