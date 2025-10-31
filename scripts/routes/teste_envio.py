from flask import Blueprint, request, jsonify
from fake_lora import send_json_to_serial, wrap_payload

teste_envio_bp = Blueprint('teste_envio', __name__)

PORT = '/dev/ttyUSB0'  # Atualize conforme necessário

@teste_envio_bp.route('/teste_envio', methods=['POST'])
def teste_envio():
    # Try to get JSON regardless of Content-Type
    data = request.get_json(force=True, silent=True)
    if data is None:
        return jsonify({"error": "No JSON data provided"}), 400

    avionics_data = wrap_payload(data);
    send_json_to_serial(avionics_data, port=PORT, baudrate=9600)

    # Process the data as needed
    # For demonstration, we will just return the received data
    return jsonify({"SUCCESS!!": data}), 200