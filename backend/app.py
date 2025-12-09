# backend/app.py

from flask import Flask, request, jsonify
import os
import time
# Import all three utility files
from utils.cv_processor import cv_processor
from utils.distance import estimate_distance
from utils.inverse_kinematics import calculate_ik_angles

app = Flask(__name__)

# --- Configuration ---
DATA_FOLDER = 'data'
if not os.path.exists(DATA_FOLDER):
    os.makedirs(DATA_FOLDER)

is_garbage_detected = False

@app.route('/log', methods=['POST'])
def receive_log():
    """Receives and prints debug messages from the ESP32-CAM."""
    if request.method == 'POST':
        try:
            log_message = request.data.decode('utf-8')
            print(f"[ESP32 LOG]: {log_message}")
            return jsonify({"status": "received"}), 200
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500
    return jsonify({"status": "error"}), 400

@app.route('/upload', methods=['POST'])
def upload_image():
    global is_garbage_detected
    
    if is_garbage_detected:
        # If detected, tell the ESP32 to stop (or confirm stop)
        return jsonify({"status": "STOP", "message": "Trash found. Robot moving."}), 200

    if request.method == 'POST':
        if not request.data:
            return jsonify({"status": "error", "message": "No image data received"}), 400
        
        timestamp = time.strftime("%Y%m%d-%H%M%S")
        filename = os.path.join(DATA_FOLDER, f"capture_{timestamp}.jpg")
        
        try:
            # --- 1. Save Image ---
            with open(filename, 'wb') as f:
                f.write(request.data)
            
            # --- 2. Detection (cv_processor.py) ---
            trash_found, coords_px = cv_processor.detect_trash(filename)
            
            if trash_found:
                # --- 3. Distance Estimation (distance.py) ---
                width_pixels = coords_px["width_pixels"]
                distance_cm = estimate_distance(width_pixels)
                
                if distance_cm is None:
                     return jsonify({"status": "CONTINUE", "message": "Trash found but distance failed."}), 200

                # --- 4. Inverse Kinematics (inverse_kinematics.py) ---
                servo_angles = calculate_ik_angles(coords_px, distance_cm)

                if servo_angles is None:
                     return jsonify({"status": "CONTINUE", "message": "Trash found but target unreachable."}), 200

                # --- 5. Success! Prepare Final Response ---
                is_garbage_detected = True 
                print(f"IK Success. Angles: {servo_angles}")
                
                # Send the final command (angles) and stop signal to ESP32
                return jsonify({
                    "status": "DETECTED_AND_COMMAND", 
                    "stop_capture": True,
                    "target_angles": servo_angles,  # <--- Send the calculated angles!
                    "reach_cm": f"{distance_cm:.2f}"
                }), 200
            
            # --- 6. No Trash Found ---
            return jsonify({
                "status": "CONTINUE", 
                "stop_capture": False,
                "message": "No trash detected, continue capture."
            }), 200

        except Exception as e:
            print(f"CRITICAL ERROR: {e}")
            return jsonify({"status": "error", "message": str(e)}), 500


if __name__ == '__main__':
    # Start the Flask server
    app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)