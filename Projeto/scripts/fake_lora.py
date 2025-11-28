import serial
import json
import time

# --- 1. CONFIGURAÇÃO ---
# ⚠️ IMPORTANTE: Altere esta variável para a sua porta serial correta!
PORTA_SERIAL = '/dev/ttyUSB0'
BAUDRATE = 9600

# --- 2. DADOS PARA ENVIO (PAYLOADS) ---
# Criamos uma lista de dicionários Python. Cada dicionário será convertido para um JSON.

mensagens_para_enviar = [
    # Exemplo 1 (fornecido por você)
    { 
        "equipe": "1", 
        "payload": { 
            "temperatura": 12.7, 
            "pressao": 980.1,
            "umidade_relativa": 65.2,
            "sdr": "68abfd7b155cc0fc023a052"
        } 
    },
    
    # Exemplo 2 (corrigido por nós)
    { 
        "equipe": "2", 
        "payload": { 
            "flight_time_s": 122, 
            "altitude_m": 1540,
            "bateria_V": 3.98,
            "gyroscope": {
                "x": 1.05,
                "y": -2.1,
                "z": 0.3
            },
            "sdr": "68abfd90155cc0fc423b0429"
        } 
    },
    
    # --- Mais 8 exemplos criados ---
    
    # Exemplo 3: Equipe 3, com dados de GPS
    {
        "equipe": "3",
        "payload": {
            "gps": {
                "latitude": -7.1195,
                "longitude": -34.8449,
                "sats": 8
            },
            "status": "Aguardando lançamento",
            "sdr": "68abfda613d620fc353a1c2a"
        }
    },

    # Exemplo 4: Equipe 4, com dados de acelerômetro
    {
        "equipe": "4",
        "payload": {
            "flight_time_s": 35.2,
            "acelerometro": {
                "x": 0.1,
                "y": 0.05,
                "z": 9.81
            },
            "bateria_V": 4.1,
            "sdr": "68abfdd013d620fc1139082a"   
        }
    },

    # Exemplo 5: Equipe 1, atualização de estado
    {
        "equipe": "1",
        "payload": {
            "temperatura": 15.1,
            "pressao": 975.5,
            "status_voo": "ASCENDENTE",
            "sdr": "68abfde613d620fc05390a28"
        }
    },

    # Exemplo 6: Equipe 5, com dados booleanos (verdadeiro/falso)
    {
        "equipe": "5",
        "payload": {
            "altitude_m": 2500,
            "paraquedas_acionado": True,
            "missao_completa": False,
            "sdr": "68abfe01155cc0fc0c3c022c"
        }
    },

    # Exemplo 7: Equipe 3, atualização GPS e bateria
    {
        "equipe": "3",
        "payload": {
            "gps": {
                "latitude": -7.1201,
                "longitude": -34.8555,
                "sats": 9
            },
            "bateria_V": 3.7,
            "sdr": "68abfe1710c8e0fc0a2f062c"
        }
    },

    # Exemplo 8: Equipe 2, dados de apogeu
    {
        "equipe": "2",
        "payload": {
            "flight_time_s": 180,
            "altitude_maxima_m": 3102,
            "status_sistema": "APOGEU ATINGIDO",
            "sdr": "68abfe2d0dbba0fc012a0336"
        }
    },
    
    # Exemplo 9: Equipe 4, dados de pouso
    {
        "equipe": "4",
        "payload": {
            "flight_time_s": 450.7,
            "acelerometro": {
                "x": 1.2,
                "y": -0.8,
                "z": 4.5
            },
            "status_final": "POUSO DETECTADO",

        }
    },

    # Exemplo 10: Equipe 5, pacote final
    {
        "equipe": "5",
        "payload": {
            "altitude_m": 5,
            "paraquedas_acionado": True,
            "missao_completa": True,
            "recuperacao": "SINAL ATIVO",
            "sdr": "68abfe4013d620fc463a092a"
        }
    },
]

while True:
	# --- 3. LÓGICA DE ENVIO SERIAL ---
    ser = None  # Inicializa a variável fora do try
    try:
	    # Abre a porta serial
        ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=1)
        print(f"Porta serial {PORTA_SERIAL} aberta com sucesso. Iniciando transmissão...")
        time.sleep(2) # Pequena pausa antes de começar

	    # Itera sobre cada mensagem na nossa lista
        for i, mensagem in enumerate(mensagens_para_enviar):
            # Converte o dicionário Python para uma string no formato JSON
            # O argumento `separators` remove espaços desnecessários, economizando bytes
            string_json = json.dumps(mensagem, separators=(',', ':'))
            print(f"Enviando mensagem {i+1}/{len(mensagens_para_enviar)} -> {string_json}")
		
            # Prepara os dados para envio:
            # 1. Codifica a string para bytes (padrão utf-8)
            # 2. Adiciona um caractere de nova linha (b'\n') no final.
            #    Isso é CRUCIAL para que o receptor saiba onde a mensagem termina!
            dados_para_enviar = string_json.encode('utf-8') + b'\n'
            
            # Escreve os bytes na porta serial
            ser.write(dados_para_enviar)
            
            # Espera 3 segundos antes de enviar a próxima mensagem
            time.sleep(1)
        print("\nTodas as mensagens foram enviadas.")
    except serial.SerialException as e:
        print(f"ERRO: Não foi possível abrir ou escrever na porta serial {PORTA_SERIAL}.")
        print(f"Detalhes: {e}")
    except Exception as e:
        print(f"Ocorreu um erro inesperado: {e}")
    finally:
	    # Este bloco SEMPRE será executado, garantindo que a porta seja fechada
	    if ser and ser.is_open:
		    ser.close()
		    print("Porta serial fechada.")

def wrap_payload(data):
    """
    Wraps the payload in the required structure.
    Args:
        data (dict): The original data containing "equipe" and other keys.
    Returns:
        dict: The wrapped data in the format {"equipe": <equipe>, "payload": {...}}
    """

    if not isinstance(data, dict):
        raise ValueError("Input must be a dictionary")
    equipe = data.get("equipe")
    payload = {k: v for k, v in data.items() if k != "equipe"}
    return {"equipe": equipe, "payload": payload}

def send_json_to_serial(data, port='COM4', baudrate=9600):
    """
    Sends a JSON-serializable dictionary to the specified serial port.
    Args:
        data (dict): The data to send.
        port (str): The serial port to use (e.g., 'COM4' or '/dev/ttyUSB0').
        baudrate (int): The baud rate for the serial communication
    """

    wrapped_data = wrap_payload(data)
    json_string = json.dumps(wrapped_data, separators=(',', ':'))
    data_to_send = json_string.encode('utf-8') + b'\n'

    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            time.sleep(2)  # Wait for the connection to establish
            ser.write(data_to_send)
            print(f"Sent: {json_string}")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")