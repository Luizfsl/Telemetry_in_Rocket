from flask import Flask, request, jsonify
from routes.teste_envio import teste_envio
from fake_lora import send_json_to_serial, wrap_payload

app = Flask(__name__)

# Register the 'teste_envio' route
app.add_url_rule('/teste_envio', view_func=teste_envio, methods=['POST'])

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)