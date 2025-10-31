document.addEventListener('DOMContentLoaded', () => {

    const loraStatusIcon = document.getElementById('lora-status');

    // --- SIMULAÇÃO PARA DEMONSTRAÇÃO ---
    // Simula o recebimento de dados a cada 5 segundos
    console.log("Iniciando simulação...");

    window.startLoraAnimation = function() {
        console.log("Recebendo dados... Ativando animação.");
        loraStatusIcon.classList.remove('static');
        loraStatusIcon.classList.add('receiving');

        // Para a animação após um tempo (ex: 2 segundos)
        setTimeout(() => {
            stopAnimation();
        }, 4000); // A animação dura 2s, depois volta ao estado estático
    }

    function stopAnimation() {
        console.log("Parou de receber. Desativando animação.");
        loraStatusIcon.classList.remove('receiving');
        loraStatusIcon.classList.add('static');
    }
});