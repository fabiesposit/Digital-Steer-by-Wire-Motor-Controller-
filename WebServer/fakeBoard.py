from flask import Flask
import time
import math
import random

app = Flask(__name__)

start_time = time.time()

def get_simulated_values():
    """
    Genera valori fittizi ma realistici per:
    - posizione attuale
    - posizione richiesta
    - pwm
    """
    t = time.time() - start_time

    # posizione attuale in gradi, oscilla tra -180 e 180
    actual = 180 * math.sin(2 * math.pi * 0.1 * t)

    # posizione richiesta sfasata rispetto alla actual
    requested = 180 * math.sin(2 * math.pi * 0.1 * t + math.pi / 4)

    # pwm in range -500..+500
    pwm = 500 * math.sin(2 * math.pi * 0.5 * t)
    pwm += random.randint(-30, 30)  # un po' di rumore
    pwm = int(max(min(pwm, 500), -500))  # clamp

    return actual, requested, pwm


@app.route('/actual')
def get_actual():
    actual, _, _ = get_simulated_values()
    # risposta SOLO testo, come si aspetta il webServer
    return f"{actual:.3f}\n"


@app.route('/requested')
def get_requested():
    _, requested, _ = get_simulated_values()
    return f"{requested:.3f}\n"


@app.route('/pwm')
def get_pwm():
    _, _, pwm = get_simulated_values()
    return f"{pwm:d}\n"


if __name__ == "__main__":
    print("FAKE STM32 BOARD RUNNING ON PORT 5001")
    # Deve ascoltare sulla stessa porta che usi nel webServer (5001)
    # e su un IP raggiungibile come 192.168.1.2 dal webServer.
    app.run(host="0.0.0.0", port=5001, debug=True)
