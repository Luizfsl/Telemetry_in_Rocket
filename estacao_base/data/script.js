var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

const giroscopio_x = document.getElementById("gyro-x");
const giroscopio_y = document.getElementById("gyro-y");
const giroscopio_z = document.getElementById("gyro-z");
const ax = document.getElementById("accx");
const ay = document.getElementById("accy");
const az = document.getElementById("accz");
const magnetometro_x = document.getElementById("mag-x");
const magnetometro_y = document.getElementById("mag-y");
const magnetometro_z = document.getElementById("mag-z");
const battery = document.getElementById("battery");
const pressure = document.getElementById("pressure");
const temperature = document.getElementById("temp");
const humidity = document.getElementById("humidity");

const baseLat = document.getElementById("base-lat");
const baseLon = document.getElementById("base-lon");
const baseAltitude = document.getElementById("base-altitude");
const baseHdop = document.getElementById("base-hdop");
const baseSat = document.getElementById("base-sat");

const sdr_freq_center = document.getElementById("sdr_freq_center");
const sdr_timestamp = document.getElementById("sdr_timestamp");
const sdr_offset = document.getElementById("sdr_offset");
const sdr_potencia = document.getElementById("sdr_potencia");
const sdr_bandwidth = document.getElementById("sdr_bw");
const sdr_snr = document.getElementById("sdr_snr");

// Exemplo de botão para sintonia usando o valor do input number
const sintoniaButton = document.getElementById("sintonia-button");
const channelInput = document.getElementById("canal-slider");
const freqElement = document.getElementById("frequencia-atual");

const altitude = document.getElementById("altitude");
const avionicLat = document.getElementById("avionic-lat");
const avionicLon = document.getElementById("avionic-lon");
const avionicGpsAltitude = document.getElementById("avionic-gps-altitude");
const avionicSpeed = document.getElementById("avionic-gps-speed");
const avionicCourse = document.getElementById("avionic-course");
const avionicDistance = document.getElementById("avionic-distance");
const avionicHdop = document.getElementById("avionic-hdop");
const avionicSat = document.getElementById("avionic-sat");

const ascendingStatus = document.getElementById("status-ascending");
const drogueStatus = document.getElementById("status-drogue");
const mainStatus = document.getElementById("status-main");
const sysTimer = document.getElementById("status-timer");
const battVoltage = document.getElementById("status-vbatt");

const sdStatus = document.getElementById("status-sd");
const gpsStatus = document.getElementById("status-gps");
const mpuStatus = document.getElementById("status-mpu");
const mplStatus = document.getElementById("status-mpl");
const logStatus = document.getElementById("status-log");
const drogueButton = document.getElementById("drogue-button");
const mainButton = document.getElementById("main-button");
const rstSysButton = document.getElementById("rst-button");
const rstTimerButton = document.getElementById("rst-timer-button");
const startButton = document.getElementById("start-button");
const stopButton = document.getElementById("stop-button");

let sessaoAtiva = false;
let chaveSessao = '';
let dadosSessao = [];

window.addEventListener('load', onload);

function onload() {
	initWebSocket();
	initEventListeners();
}

function getGpsPrecision(hdop, domElement) {
	if(hdop <= 1.0) {
		domElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");
		domElement.classList.add("valor-verde");
		return 'Excelente';
	}
	else if(hdop > 1.0 && hdop <= 2.0) {
		domElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");
		domElement.classList.add("valor-verde");
		return 'Muito boa';
	}
	else if(hdop > 2.0 && hdop <= 5.0) {
		domElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");
		domElement.classList.add("valor-amarelo");
		return 'Moderada';
	}
	else if(hdop > 5.0 && hdop <= 10) {
		domElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");
		domElement.classList.add("valor-vermelho");
		return 'Ruim';
	}
	else {
		domElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");
		domElement.classList.add("valor-vermelho");
		return 'Péssima';
	}
}

// função de inicialização do websocket
function initWebSocket() {
	console.log('Iniciando websocket…');
	websocket = new WebSocket(gateway);
	websocket.onopen = onOpen;
	websocket.onclose = onClose;
	websocket.onmessage = onMessage;
	websocket.binaryType = 'arraybuffer';
}

function initEventListeners() {
	rstSysButton.addEventListener("click", function(){
		console.log("Resetando o sistema...");
		const buffer = new ArrayBuffer(3);
		const dataView = new DataView(buffer);
		dataView.setUint8(0, 3);
		websocket.send(buffer);
  	});

	startButton.addEventListener("click", function(){
		if (sessaoAtiva) {
			alert("Já existe uma sessão de log ativa.");
			return;
		}

		const agora = new Date();
		let hora = agora.toLocaleTimeString('pt-BR').replaceAll(':', '-');
		let data = agora.toLocaleDateString('pt-BR').replaceAll('/', '-');
		chaveSessao = data + '-' + hora;
		dadosSessao = [];
		sessaoAtiva = true;
		logStatus.classList.remove("valor-verde", "valor-vermelho");
		logStatus.classList.add("valor-verde");
		logStatus.innerHTML = "Ativo";

		alert("Sessão iniciada em: " + chaveSessao);
	});

	stopButton.addEventListener("click", function(){
		if (!sessaoAtiva) {
			alert("Nenhuma sessão de log ativa.");
			return;
		}

		const csv = gerarCSV(dadosSessao);
		const nomeArquivo = `sessao_${chaveSessao.replace(/[:.]/g, '-')}.csv`;

		const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });
		const url = URL.createObjectURL(blob);

		const a = document.createElement("a");
		a.href = url;
		a.download = nomeArquivo;
		a.style.display = "none";
		document.body.appendChild(a);
		a.click();
		document.body.removeChild(a);

		URL.revokeObjectURL(url);

		sessaoAtiva = false;
		logStatus.classList.remove("valor-verde", "valor-vermelho");
		logStatus.classList.add("valor-vermelho");
		logStatus.innerHTML = "Inativo";
		chaveSessao = '';
		dadosSessao = [];

		alert("Sessão encerrada e arquivo CSV baixado.");
	});

	sintoniaButton.addEventListener("click", function() {
		const canal = channelInput.value;
		if (canal !== "" && !isNaN(canal)) {
			enviarSintonia(Number(canal));
		} else {
			alert("Informe um canal válido.");
		}
	});
}

function adicionarDado(dado) {
	if (!sessaoAtiva) return;

	dadosSessao.push(dado);
}

function gerarCSV(dados) {
	if (dados.length === 0) return "";

	const headers = Object.keys(dados[0]).join(",");
	const linhas = dados.map(d => Object.values(d).join(","));
	return [headers, ...linhas].join("\n");
}

/**
 * Estima a porcentagem de carga de um pack de baterias Li-ion 2S (2x 18650).
 * A função utiliza uma curva de descarga não-linear aproximada por múltiplos segmentos
 * para fornecer uma estimativa mais precisa da carga restante.
 *
 * @param {number} tensao A tensão atual do pack, medida em Volts (faixa esperada: 6.0V a 8.4V).
 * @returns {number} A porcentagem de carga estimada (0.0 a 100.0).
 */
function tensaoParaPorcentagem(tensao) {
	// Garante que a tensão esteja dentro dos limites para evitar cálculos anormais.
	if (tensao >= 8.4) {
		return 100.0;
	}
	if (tensao <= 6.0) {
		return 0.0;
	}

	if (tensao > 8.0) {
		// Segmento superior: 8.4V (100%) a 8.0V (90%)
		return 90 + (tensao - 8.0) * (100 - 90) / (8.4 - 8.0);
	} else if (tensao > 7.5) {
		// Segmento meio-superior: 8.0V (90%) a 7.5V (50%)
		return 50 + (tensao - 7.5) * (90 - 50) / (8.0 - 7.5);
	} else if (tensao > 7.1) {
		// Segmento meio-inferior: 7.5V (50%) a 7.1V (20%)
		return 20 + (tensao - 7.1) * (50 - 20) / (7.5 - 7.1);
	} else if (tensao > 6.8) {
		// Segmento inferior: 7.1V (20%) a 6.8V (5%)
		return 5 + (tensao - 6.8) * (20 - 5) / (7.1 - 6.8);
	} else { // se a tensão for > 6.0
		// Segmento final: 6.8V (5%) a 6.0V (0%)
		return 0 + (tensao - 6.0) * (5 - 0) / (6.8 - 6.0);
	}
}

function enviarSintonia(canal) {
	if (websocket && websocket.readyState === WebSocket.OPEN) {
		const comando = {
			acao: "sintonizar",
			canal: canal
		};
		websocket.send(JSON.stringify(comando));
		console.log("Comando de sintonia enviado:", comando);
	} else {
		alert("WebSocket não está conectado.");
	}
}

function onOpen() {    
	console.log('Conexão aberta.');
	while(websocket.readyState !== WebSocket.OPEN){ 
		/* ESPERA ESTAR CONECTADO */ 
	}
    console.log('Conectado.')
}

function onClose() {
	console.log('Conexão fechada.');
	setTimeout(initWebSocket, 1000);
}

function onMessage(event) {
	// Adicionando ao terminal
	window.addLogToPitubasTerminal(event.data);
	
	const data = JSON.parse(event.data);	
	console.log('JSON recebido via websocket:', data);	
	
	if (data && data.hasOwnProperty('resultado_sintonia')) {
		// O E220-900T30D opera na faixa de 862~931 MHz, com espaçamento de 1 MHz por canal.
		// Canal 0 = 862 MHz, Canal 1 = 863 MHz, ..., Canal 69 = 931 MHz.
		const resultado = data.resultado_sintonia;
		if (!freqElement) return;
		
		freqElement.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");

		if (resultado === true) {
			const frequencia = 850 + data.canal; // MHz
			freqElement.classList.add("valor-verde");
			freqElement.innerHTML = `${frequencia} MHz`;
		} else {
			freqElement.classList.add("valor-vermelho");
		}
		return;
	}

	if (!data.hasOwnProperty('equipe') || !data.hasOwnProperty('payload')) {
		return;
	}

	//Recebendo dados, então chame a animação do LoRa
	window.startLoraAnimation();

	const equipe = data.equipe;
	const payload = data.payload;

	const acelerometro = payload.acelerometro;
	const temperatura = payload.temperatura;
	const giroscopio = payload.giroscopio;
	const magnetometro = payload.magnetometro;
	const pressao = payload.pressao;
	const umidade = payload.umidade | payload.umidade_relativa;
	const tensao_bateria = payload.bateria;
	const gps = payload.gps;
	const sdr_hex = payload.sdr;

	console.info("Equipe = ", equipe);
	console.info("Payload = ", payload);
	console.info("Temperatura = ", temperatura);
	console.info("Acelerometro = ", acelerometro);
	console.info("Giroscopio = ", giroscopio);
	console.info("Magnetometro = ", magnetometro);
	console.info("Pressao = ", pressao);
	console.info("Umidade = ", umidade);
	console.info("Bateria = ", tensao_bateria);
	console.info("GPS = ", gps);
	console.info("SDR (hex) = ", sdr_hex);

	if (acelerometro !== undefined) {
		ax.innerHTML = Number(acelerometro.x).toFixed(2);
		ay.innerHTML = Number(acelerometro.y).toFixed(2);
		az.innerHTML = Number(acelerometro.z).toFixed(2);
	}

	if (giroscopio !== undefined) {
		giroscopio_x.innerHTML = Number(giroscopio.x).toFixed(2);
		giroscopio_y.innerHTML = Number(giroscopio.y).toFixed(2);
		giroscopio_z.innerHTML = Number(giroscopio.z).toFixed(2);
	}

	if (magnetometro !== undefined) {
		magnetometro_x.innerHTML = Number(magnetometro.x).toFixed(2);
		magnetometro_y.innerHTML = Number(magnetometro.y).toFixed(2);
		magnetometro_z.innerHTML = Number(magnetometro.z).toFixed(2);
	}

	if (temperatura !== undefined) {
		temperature.innerHTML = temperatura + ' °C';
	}

	if (pressao !== undefined) {
		pressure.innerHTML = pressao + ' hPa';
	}

	if (umidade !== undefined) {
		humidity.innerHTML = umidade + ' %';
	}

	if (tensao_bateria !== undefined) {
		percentual_bateria = tensaoParaPorcentagem(tensao_bateria);
		battVoltage.innerHTML = tensao_bateria.toFixed(2) + ' V';
		battery.innerHTML = percentual_bateria + ' %';
		battery.classList.remove("valor-verde", "valor-amarelo", "valor-vermelho");

		if(percentual_bateria >= 75.0) {			
			battery.classList.add("valor-verde");
		}
		else if(percentual_bateria >= 35 && percentual_bateria < 75) {
			battery.classList.add("valor-amarelo");
		}
		else {
			battery.classList.add("valor-vermelho");
		}
	}

	if(gps !== undefined) {
		baseLat.innerHTML = gps.latitude.toFixed(6) + ' °';
		baseLon.innerHTML = gps.longitude.toFixed(6) + ' °';
		baseHdop.innerHTML = gps.precision;
		baseSat.innerHTML = gps.satellites;
	}

	if (sdr_hex !== undefined) {
		const sdrReport = parseSdrReport(sdr_hex);
		if (sdrReport) {
			console.log("Relatório SDR decodificado:", sdrReport);
			// Aqui você pode atualizar o DOM com os dados do SDR, se desejar.
		} else {
			console.warn("Falha ao decodificar o relatório SDR.");
		}

		sdr_freq_center.innerHTML = sdrReport.center_freq_hz;
		sdr_timestamp.innerHTML = sdrReport.timestamp;
		sdr_offset.innerHTML = sdrReport.signals[0].offset_hz;
		sdr_bandwidth.innerHTML = sdrReport.signals[0].bandwidth_hz;
		sdr_potencia.innerHTML = sdrReport.signals[0].power_dbm;
		sdr_snr.innerHTML = sdrReport.signals[0].snr_db;
	}

	let _ax, _ay, _az, _roll, _pitch, _yaw, _altitude, _pressure, _temperature;
	let _apogee, _gpsLat, _gpsLon, _gpsSpeed, _gpsAltitude, _gpsHdop;
	let _baseGpsDistance, _baseGpsCourse, _baseGpsLat, _baseGpsLon; 
	let _baseGpsAltitude, _baseGpsHdop, _vx, _vy, _vz, _vBattery;
	let _sys_hour, _sys_min, _sys_sec, _gpsSat, _baseGpsSat, _statusMpu;
	let _statusMpl, _statusGps, _statusSd, _main, _drogue, _ascending, _lowBatt;

	const dataView = new DataView(event.data);
	console.log(dataView.byteLength + ' bytes recebidos via websocket.');


	if (dataView.byteLength >= 113) {/*Tamanho da struct completa enviada pelo esp*/
		
		_ax = dataView.getFloat32(0, true).toFixed(2);
		ax.innerHTML = _ax;

		_ay = dataView.getFloat32(4, true).toFixed(2);
		ay.innerHTML = _ay + ' m/s²';

		_az = dataView.getFloat32(8, true).toFixed(2);
		az.innerHTML = _az + ' m/s²';

		_roll = dataView.getFloat32(12, true).toFixed(2);
		magnetometro_x.innerHTML = _roll + ' °';

		_pitch = dataView.getFloat32(16, true).toFixed(2);
		magnetometro_y.innerHTML = _pitch + ' °';

		_yaw = dataView.getFloat32(20, true).toFixed(2);
		magnetometro_z.innerHTML = _yaw + ' °';

		_altitude = dataView.getFloat32(24, true).toFixed(2);
		altitude.innerHTML = _altitude + ' m';

		_pressure = dataView.getFloat32(28, true).toFixed(2);
		pressure.innerHTML = (_pressure * 0.1) + ' kPa';

		_temperature = dataView.getFloat32(32, true).toFixed(2);
		temperature.innerHTML = _temperature + ' °C';

		_apogee = dataView.getFloat32(36, true).toFixed(2);
		umidade.innerHTML = _apogee + ' m';

		_gpsLat = dataView.getFloat32(40, true).toFixed(2);
		avionicLat.innerHTML = _gpsLat + ' °';

		_gpsLon = dataView.getFloat32(44, true).toFixed(2);
		avionicLon.innerHTML = _gpsLon + ' °';

		_gpsSpeed = dataView.getFloat32(48, true).toFixed(2);
		avionicSpeed.innerHTML = _gpsSpeed + ' m/s';

		_gpsAltitude = dataView.getFloat32(52, true).toFixed(2);
		avionicGpsAltitude.innerHTML = _gpsAltitude + ' m';

		_gpsHdop = dataView.getFloat32(56, true).toFixed(2);
		avionicHdop.innerHTML = getGpsPrecision(_gpsHdop, avionicHdop);

		_baseGpsDistance = dataView.getFloat32(60, true).toFixed(2);
		avionicDistance.innerHTML = _baseGpsDistance + ' m';

		_baseGpsCourse = dataView.getFloat32(64, true).toFixed(4);
		avionicCourse.innerHTML = _baseGpsCourse + ' °';

		_baseGpsLat = dataView.getFloat32(68, true).toFixed(6);
		baseLat.innerHTML = _baseGpsLat + ' °';

		_baseGpsLon = dataView.getFloat32(72, true).toFixed(6);
		baseLon.innerHTML = _baseGpsLon + ' °';

		_baseGpsAltitude = dataView.getFloat32(76, true).toFixed(2);
		baseAltitude.innerHTML = _baseGpsAltitude + ' m';

		_baseGpsHdop = dataView.getFloat32(80, true).toFixed(2);
		baseHdop.innerHTML = getGpsPrecision(_baseGpsHdop, baseHdop);

		_vx = dataView.getFloat32(84, true).toFixed(2);
		giroscopio_x.innerHTML = _vx + ' m/s';

		_vy = dataView.getFloat32(88, true).toFixed(2);
		giroscopio_y.innerHTML = _vy + ' m/s';

		_vz = dataView.getFloat32(92, true).toFixed(2);
		giroscopio_z.innerHTML = _vz + ' m/s';

		_vBattery = dataView.getFloat32(96, true).toFixed(2);
		battVoltage.innerHTML = _vBattery + ' V';

		_gpsSat = dataView.getUint8(100, true);
		avionicSat.innerHTML = _gpsSat;

		_baseGpsSat = dataView.getUint8(101, true);
		baseSat.innerHTML = _baseGpsSat;

		_sys_hour = dataView.getUint8(102, true);
		_sys_min = dataView.getUint8(103, true);
		_sys_sec = dataView.getUint8(104, true);

		sysTimer.innerHTML = _sys_hour +':'+ _sys_min +':'+ _sys_sec;

		_statusMpu = dataView.getUint8(105, true);
		if(_statusMpu == 1) {
			mpuStatus.classList.remove("valor-verde", "valor-vermelho");
			mpuStatus.classList.add("valor-verde");
			mpuStatus.innerHTML = 'Ativo';
		}
		else {
			mpuStatus.classList.remove("valor-verde", "valor-vermelho");
			mpuStatus.classList.add("valor-vermelho");
			mpuStatus.innerHTML = 'Inativo';
		}

		_statusMpl = dataView.getUint8(106, true);
		if(_statusMpl == 1) {
			mplStatus.classList.remove("valor-verde", "valor-vermelho");
			mplStatus.classList.add("valor-verde");
			mplStatus.innerHTML = 'Ativo';
		}
		else {
			mplStatus.classList.remove("valor-verde", "valor-vermelho");
			mplStatus.classList.add("valor-vermelho");
			mplStatus.innerHTML = 'Inativo';
		}

		_statusGps = dataView.getUint8(107, true);
		if(_statusGps == 1) {
			gpsStatus.classList.remove("valor-verde", "valor-vermelho");
			gpsStatus.classList.add("valor-verde");
			gpsStatus.innerHTML = 'Com sinal';
		}
		else {
			gpsStatus.classList.remove("valor-verde", "valor-vermelho");
			gpsStatus.classList.add("valor-vermelho");
			gpsStatus.innerHTML = 'Sem sinal';
		}

		_statusSd = dataView.getUint8(108, true);
		if(_statusSd == 1) {
			sdStatus.classList.remove("valor-verde", "valor-vermelho");
			sdStatus.classList.add("valor-verde");
			sdStatus.innerHTML = 'Ativo';
		}
		else {
			sdStatus.classList.remove("valor-verde", "valor-vermelho");
			sdStatus.classList.add("valor-vermelho");
			sdStatus.innerHTML = 'Inativo';
		}

		_main = dataView.getUint8(109, true);
		if(_main == 1) {
			mainStatus.classList.remove("valor-verde", "valor-vermelho");
			mainStatus.classList.add("valor-verde");
			mainStatus.innerHTML = 'Acionado';
		}
		else {
			mainStatus.classList.remove("valor-verde", "valor-vermelho");
			mainStatus.classList.add("valor-vermelho");
			mainStatus.innerHTML = 'Não acionado';
		}

		_drogue = dataView.getUint8(110, true);
		if(_drogue == 1) {
			drogueStatus.classList.remove("valor-verde", "valor-vermelho");
			drogueStatus.classList.add("valor-verde");
			drogueStatus.innerHTML = 'Acionado';
		}
		else {
			drogueStatus.classList.remove("valor-verde", "valor-vermelho");
			drogueStatus.classList.add("valor-vermelho");
			drogueStatus.innerHTML = 'Não acionado';
		}

		_ascending = dataView.getUint8(111, true);
		if(_ascending == 1) {
			ascendingStatus.innerHTML = 'Subindo';
		}
		else {
			ascendingStatus.innerHTML = 'Descendo';
		}

		_lowBatt = dataView.getUint8(112, true);
		if(_lowBatt == 1) {
			statusBatt.classList.remove("valor-verde", "valor-vermelho");
			statusBatt.classList.add("valor-vermelho");
			statusBatt.innerHTML = 'Bateria fraca';
		}
		else {
			statusBatt.classList.remove("valor-verde", "valor-vermelho");
			statusBatt.classList.add("valor-verde");
			statusBatt.innerHTML = 'Com carga';
		}

		let dado = {
			ax: _ax,
			ay: _ay,
			az: _az,
			vx: _vx,
			vy: _vy,
			vz: _vz,
			roll: _roll,
			pitch: _pitch,
			yaw: _yaw,
			altitude: _altitude,
			pressure: _pressure,
			temperature: _temperature,
			apogee: _apogee,
			gpsLat: _gpsLat,
			gpsLon: _gpsLon,
			gpsSpeed: _gpsSpeed,
			gpsAltitude: _gpsAltitude,
			gpsHdop: _gpsHdop,
			gpsSat: _gpsSat,
			baseGpsDistance: _baseGpsDistance,
			baseGpsCourse: _baseGpsCourse,
			baseGpsLat: _baseGpsLat,
			baseGpsLon: _baseGpsLon,
			baseGpsAltitude: _baseGpsAltitude,
			baseGpsHdop: _baseGpsHdop,
			baseGpsSat: _baseGpsSat,
			vBattery: _vBattery,
			sysHour: _sys_hour,
			sysMin: _sys_min,
			sysSec: _sys_sec,
			main: _main,
			drogue: _drogue,
			ascending: _ascending
		};

		adicionarDado(dado);
	}
};

/* ================================================= */
/* SCRIPT DO COMPONENTE TERMINAL PITUBAS (ISOLADO) */
/* ================================================= */

// Envolvemos o código para garantir que ele rode após o HTML ser carregado
// e para não poluir o escopo global.
document.addEventListener('DOMContentLoaded', () => {

    // Seleciona os elementos específicos DENTRO do nosso componente
    const terminalComponent = document.querySelector('.pitubas-terminal-component');
    
    // Se o componente não existir na página, o script para aqui.
    if (!terminalComponent) {
        console.warn('Componente Terminal Pitubas não encontrado na página.');
        return;
    }

    const logDisplay = terminalComponent.querySelector('.pt-log-display');
    const downloadBtn = terminalComponent.querySelector('.pt-download-btn');

    let logDataStorage = []; // Array para guardar os logs para download

    /**
     * Adiciona uma nova entrada de log ao console visual e armazena os dados.
     * @param {string} jsonString - O JSON recebido como string.
     */
    window.addLogToPitubasTerminal = function(jsonString) {
        if (!logDisplay) return; // Segurança extra

        logDataStorage.push(jsonString); // Guarda o JSON original

        let contentToDisplay;
        try {
            const dataObject = JSON.parse(jsonString);
            contentToDisplay = JSON.stringify(dataObject, null, 2); // Formata para leitura
        } catch (e) {
            contentToDisplay = `[ERRO AO LER JSON]: ${jsonString}`;
        }

        const timestamp = new Date().toLocaleTimeString('pt-BR');
        
        // Adiciona o novo log ao final do display
        logDisplay.innerHTML += `<div><span style="color: #888;">[${timestamp}]</span>\n${contentToDisplay}\n\n</div>`;

        // Mantém o scroll sempre no final para mostrar a última mensagem
        logDisplay.scrollTop = logDisplay.scrollHeight;
    }

    /**
     * Gera e baixa um arquivo .txt com todos os logs da sessão.
     */
    function downloadLogs() {
        if (logDataStorage.length === 0) {
            alert("Nenhum log para baixar!");
            return;
        }

        const fileContent = logDataStorage.join('\n\n---\n\n');
        const blob = new Blob([fileContent], { type: 'text/plain;charset=utf-8' });
		const now = new Date();
		const dateStr = now.toLocaleDateString('pt-BR').replace(/\//g, '-');
		const timeStr = now.toLocaleTimeString('pt-BR').replace(/:/g, '-');
		const data_hora = `${dateStr}_${timeStr}`;
        const fileName = `${data_hora}_PITUBAS_logs.txt`;
        const link = document.createElement('a');

        link.href = URL.createObjectURL(blob);
        link.download = fileName;
        
        link.click();
        URL.revokeObjectURL(link.href);
    }

    // Vincula a função de download ao botão
    downloadBtn.addEventListener('click', downloadLogs);
});

/**
 * Decodifica uma string hexadecimal de relatório SDR para um objeto JavaScript legível.
 * @param {string} hexString A string hexadecimal recebida no JSON (ex: '68d9f1a0...').
 * @returns {object} Um objeto com os dados decodificados ou null em caso de erro.
 */
function parseSdrReport(hexString) {
    if (!hexString || hexString.length < 14) { // 14 caracteres = 7 bytes de cabeçalho
        return null;
    }

    // 1. Converte a string hexadecimal em um array de bytes (números de 0 a 255)
    const bytes = [];
    for (let i = 0; i < hexString.length; i += 2) {
        bytes.push(parseInt(hexString.substr(i, 2), 16));
    }

    // 2. Usa DataView para ler números multi-byte do buffer de forma controlada
    const buffer = new Uint8Array(bytes).buffer;
    const view = new DataView(buffer);

    // 3. Decodifica o Cabeçalho (7 bytes)
    // Lê um inteiro de 4 bytes (Big Endian) a partir do byte 0
    const timestamp = view.getUint32(0, false); 
    // Lê um inteiro de 3 bytes (24 bits) manualmente
    const freq_encoded = (view.getUint8(4) << 16) | (view.getUint8(5) << 8) | view.getUint8(6);
    const center_freq_hz = freq_encoded * 100;

    const report = {
        timestamp: timestamp,
        center_freq_hz: center_freq_hz,
        signals: []
    };

    // 4. Decodifica cada sinal (blocos de 5 bytes)
    let offset = 7; // Começa a ler após o cabeçalho
    while (offset + 5 <= buffer.byteLength) {
        // Lê um inteiro de 2 bytes COM SINAL (Big Endian)
        const offset_hz = view.getInt16(offset, false);
        const quantized_power = view.getUint8(offset + 2);
        const quantized_bw = view.getUint8(offset + 3);
        const quantized_snr = view.getUint8(offset + 4);

        // 5. Dequantiza os valores de volta para suas unidades originais
        // (Esta matemática deve ser o inverso exato do código Python)
        const power_dbm = (quantized_power / 2.0) - 100.0;
        const bandwidth_hz = quantized_bw * 100.0;
        const snr_db = quantized_snr / 4.0;

        report.signals.push({
            offset_hz: offset_hz,
            power_dbm: power_dbm.toFixed(2),
            bandwidth_hz: bandwidth_hz,
            snr_db: snr_db.toFixed(2)
        });

        offset += 5;
    }

    return report;
}

/* 
setInterval(function(){
	let min = 1.0;
	let max = 50.0;
	let dado = {
		ax: (Math.random() * (max - min) + min).toFixed(1),
		ay: (Math.random() * (max - min) + min).toFixed(1),
		az: (Math.random() * (max - min) + min).toFixed(1),
		vx: (Math.random() * (max - min) + min).toFixed(1),
		vy: (Math.random() * (max - min) + min).toFixed(1),
		vz: (Math.random() * (max - min) + min).toFixed(1),
		roll: (Math.random() * (max - min) + min).toFixed(1),
		pitch: (Math.random() * (max - min) + min).toFixed(1),
		yaw: (Math.random() * (max - min) + min).toFixed(1),
		altitude: (Math.random() * (max - min) + min).toFixed(1),
		pressure: (Math.random() * (max - min) + min).toFixed(1),
		temperature: (Math.random() * (max - min) + min).toFixed(1),
		apogee: (Math.random() * (max - min) + min).toFixed(1),
		gpsLat: (Math.random() * (max - min) + min).toFixed(1),
		gpsLon: (Math.random() * (max - min) + min).toFixed(1),
		gpsSpeed: (Math.random() * (max - min) + min).toFixed(1),
		gpsAltitude: (Math.random() * (max - min) + min).toFixed(1),
		gpsHdop: (Math.random() * (max - min) + min).toFixed(1),
		gpsSat: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsDistance: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsCourse: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsLat: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsLon: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsAltitude: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsHdop: (Math.random() * (max - min) + min).toFixed(1),
		baseGpsSat: (Math.random() * (max - min) + min).toFixed(1),
		vBattery: (Math.random() * (max - min) + min).toFixed(1),
		sysHour: (Math.random() * (max - min) + min).toFixed(1),
		sysMin: (Math.random() * (max - min) + min).toFixed(1),
		sysSec: (Math.random() * (max - min) + min).toFixed(1),
		main: (Math.random() * (max - min) + min).toFixed(1),
		drogue: (Math.random() * (max - min) + min).toFixed(1),
		ascending: (Math.random() * (max - min) + min).toFixed(1)
	};
	ax.classList.add("valor-verde");
	ax.innerHTML = dado.ax + ' m/s^2';

	ay.classList.add("valor-vermelho");
	ay.innerHTML = dado.ay + ' m/s^2';

	az.classList.add("valor-amarelo");
	az.innerHTML = dado.az + ' m/s²';

	adicionarDado(dado);
}, 200); 
*/
