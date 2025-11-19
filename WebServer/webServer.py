from flask import Flask, render_template, jsonify
import requests

controlled_board_IP = "192.168.1.2"
controlled_board_port = 5001
controlled_board_url = f"http://{controlled_board_IP}:{controlled_board_port}"

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/data/<endpoint>')
def api_data_proxy(endpoint):

    # raw data retrieving from HTTP Server on Nucleo Board
    try:
        url = f"{controlled_board_url}/{endpoint}"
        
        # GET request to the board
        response = requests.get(url, timeout=0.5) 
        
        # HTTP Server returns the numeric value
        data_value = response.text.strip()
        
        # returning the browser the json format of the data value
        return jsonify({
            "status": "success",
            "value": float(data_value) # Convertiamo in float per Chart.js
        })
        
    except requests.exceptions.RequestException as e:
        # error handling
        print(f"Errore di connessione a STM32: {e}")
        return jsonify({"status": "error", "message": "STM32 Unreachable"}), 503
    
if __name__ == '__main__':
    print(f"FLASK WEB SERVER EXECUTING ON PORT: {controlled_board_port}\n")
    app.run(debug=True, host='0.0.0.0', port=5001)