from flask import Flask, render_template, jsonify
import requests
import threading
import time

# For local testing with fakeBoard.py use localhost
# For real board use the board's IP (e.g., 192.168.1.2)
controlled_board_IP = "192.168.1.2"  
controlled_board_port = 80       
controlled_board_url = f"http://{controlled_board_IP}:{controlled_board_port}"

app = Flask(__name__)

# Cache of the last 20 complete samples
data_cache = {
    "actual": [],      # Array of 20 values
    "requested": [],   # Array of 20 values
    "pwm": [],         # Array of 20 values
    "dt_ms": 5,        # Interval between samples in milliseconds
    "timestamp": time.time()
}
cache_lock = threading.Lock()

def data_fetcher():
    """Thread that retrieves all 20 samples every 100ms from the board"""
    print("[FETCHER] Starting data fetcher thread...")
    
    while True:
        try:
            # GET request to /data endpoint which returns all 20 samples
            url = f"{controlled_board_url}/data"
            response = requests.get(url, timeout=0.5)
            if response.status_code == 200:
                data = response.json()
                print(data)
                # Save ALL 20 samples to cache
                with cache_lock:
                    data_cache["actual"] = data.get("actual", [])
                    data_cache["requested"] = data.get("requested", [])
                    data_cache["pwm"] = data.get("pwm", [])
                    data_cache["dt_ms"] = data.get("dt_ms", 5)
                    data_cache["timestamp"] = time.time()
                    
                # Debug: show how many samples were received
                """
                num_samples = len(data.get("actual", []))
                if num_samples > 0:
                    print(f"[FETCHER] Retrieved {num_samples} samples at {time.strftime('%H:%M:%S.%f')[:-3]}")
                """
            else:
                print(f"[FETCHER] HTTP error: {response.status_code}")
                
        except requests.exceptions.Timeout:
            print(f"[FETCHER] Timeout - STM32 board not responding")
        except requests.exceptions.ConnectionError as e:
            print(f"[FETCHER] Connection error - is the board at {controlled_board_url}?")
        except requests.exceptions.RequestException as e:
            print(f"[FETCHER] Request error: {e}")
        except Exception as e:
            print(f"[FETCHER] Unexpected error: {e}")
        
        # Wait 100ms before next request
        time.sleep(0.1)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/data/batch')
def api_data_batch():
    """
    Main endpoint: returns ALL 20 samples in one call.
    The frontend calls this endpoint every 100ms.
    """
    try:
        with cache_lock:
            # Verify data availability
            if len(data_cache.get("actual", [])) == 0:
                return jsonify({
                    "status": "error",
                    "message": "No data available yet - waiting for STM32 board"
                }), 503
            
            return jsonify({
                "status": "success",
                "dt_ms": data_cache["dt_ms"],
                "actual": data_cache["actual"],
                "requested": data_cache["requested"],
                "pwm": data_cache["pwm"],
                "timestamp": data_cache["timestamp"],
                "num_samples": len(data_cache["actual"])
            })
            
    except Exception as e:
        print(f"[API] Error in /api/data/batch: {e}")
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 500

# Keep legacy endpoint for compatibility (if needed)
@app.route('/api/data/<endpoint>')
def api_data_single(endpoint):
    """
    Legacy endpoint: returns only the last value.
    Deprecated - use /api/data/batch instead.
    """
    try:
        with cache_lock:
            data_array = data_cache.get(endpoint, [])
            
            if len(data_array) > 0:
                # Returns the last sample (most recent)
                value = data_array[-1]
                
                return jsonify({
                    "status": "success",
                    "value": float(value)
                })
            else:
                return jsonify({
                    "status": "error",
                    "message": "No data available"
                }), 503
            
    except Exception as e:
        print(f"[API] Error in /api/data/{endpoint}: {e}")
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 500

if __name__ == '__main__':
    print("="*60)
    print("PSR MOTOR CONTROLLER - FLASK WEB SERVER")
    print("="*60)
    print(f"Server port: 5001")
    print(f"Controlled board: {controlled_board_url}")
    print(f"Fetch interval: 100ms (10 Hz)")
    print(f"Sample resolution: 5ms (200 Hz)")
    print(f"Buffer size: 20 samples (100ms window)")
    print("="*60)
    print()
    
    # Start thread for periodic data fetching
    fetcher_thread = threading.Thread(target=data_fetcher, daemon=True)
    fetcher_thread.start()
    
    # Small delay to let the fetcher start
    time.sleep(0.5)
    
    # Start Flask server
    app.run(debug=True, host='0.0.0.0', port=5001, use_reloader=False)