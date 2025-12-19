# Telemetry_in_Rocket

Projeto de telemetria para foguete, com foco em simulação/validação
de comunicação e coleta/transmissão de dados (Contiki/Cooja), incluindo scripts de obtenção de posição e plugin de mobilidade.

---

## Estrutura do repositório

```text
Telemetry_in_Rocket/
├─ Projeto/
│  ├─ Resultados/
│  │  ├─ cooja_metrics_out/
│  │  ├─ dados_Coletados/
│  │  ├─ metricas.py
│  │  └─ Simulação_SH/
│  ├─ Script_Editor/
│  ├─ base/
│  ├─ rocket/
│  ├─ Simulation_sky.csc
│  └─ plugins/mobility/
│     ├─ java/
│     └─ lib/
└─ README.md
```

### Resultados/

Pasta destinada a obtenção e análise dos dados retirados do COOJA;

### Simulação_SH/

Basta referente aos códigos dos nós utilizados na simulação. Na pasta base/ é referente ao nó que estará com a função de estação base, ou seja, funcionará como um receptor. Na pasta rocket/ é referente ao nó que assumirá o papel do foguete, ou seja, será um nó transmissor com mobilidade.

A pasta Script_Editor/ é referente a o script em java que pegará as posições do nó/foguete pela interface e enviará para o nó. Esse código deve-se ser colocado no tools/Simulate script editor....

### plugins/mobility
Pasta referente a extensão de mobilidade no Contiki/Cooja

**2025-11-19:** 
Encontrei essa playlist para colocar o plugin de mobilidade no cooja <a href="https://www.youtube.com/playlist?list=PLjUt6bve4O-flgRCMG54sLkXyABqBTsh-">mobility</a>

**2025-12-02:** 
Todo a pasta da mobilidade está em plugins. Fiz modificações no README dessa pasta para deixar mais claro o que eu fiz de diferente a cada passo do que está lá e na playlist. 
