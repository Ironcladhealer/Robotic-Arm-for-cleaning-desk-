from flask import Flask, request, jsonify
import os
import time
from utils.cv_processor import cv_processor

app = Flask(__name__)

# Create the directory to save images if it doesn't exist
DATA_FOLDER = 'data'
if not os.path.exists(DATA_FOLDER):
    os.makedirs(DATA_FOLDER)

# Global flag to control the ESP32 (state management)
is_garbage_detected = False

@app.route('/upload', methods=['POST'])
def upload_image():
    """Receives a JPEG image POSTed by the ESP32-CAM and saves it."""
    global is_garbage_detected
    
    # 1. Check stop flag first
    if is_garbage_detected:
        # Confirm detection to ESP32 and tell it to stop
        return jsonify({"status": "STOP", "message": "Trash already found, halt capture."}), 200
    
    if request.method == 'POST':
        if not request.data:
            return jsonify({"status": "error", "message": "No image data received"}), 400
        
        timestamp = time.strftime("%Y%m%d-%H%M%S")
        filename = os.path.join(DATA_FOLDER, f"capture_{timestamp}.jpg")
        image_data = request.data
        
        try:
            # --- 2. Save Image ---
            with open(filename, 'wb') as f:
                f.write(image_data)
            
            print(f"INFO: Image saved as {filename}")

            # --- 3. Run Detection using the external processor ---
            trash_found, coordinates = cv_processor.detect_trash(filename)
            
            if trash_found:
                is_garbage_detected = True # Set the global flag
                print("--- GARBAGE DETECTED! SWITCHING TO ROBOT MODE ---")
                
                # Send the signal to stop capture and the coordinates for the arm
                return jsonify({
                    "status": "DETECTED", 
                    "stop_capture": True,
                    "target_coords": coordinates 
                }), 200
            
            # --- 4. No Trash Found ---
            return jsonify({
                "status": "CONTINUE", 
                "stop_capture": False,
                "message": "No trash detected, continue capture."
            }), 200

        except Exception as e:
            print(f"ERROR: Failed to process image: {e}")
            return jsonify({"status": "error", "message": str(e)}), 500400

# --- Running the Server ---
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True, threaded = True)