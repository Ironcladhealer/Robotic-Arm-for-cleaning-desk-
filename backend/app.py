from flask import Flask, request, jsonify
import os
import time

app = Flask(__name__)

# --- Configuration ---
# Create the directory to save images if it doesn't exist
DATA_FOLDER = 'data'
if not os.path.exists(DATA_FOLDER):
    os.makedirs(DATA_FOLDER)

@app.route('/upload', methods=['POST'])
def upload_image():
    """Receives a JPEG image POSTed by the ESP32-CAM and saves it."""
    
    if request.method == 'POST':
        # Ensure the request contains image data
        if request.data:
            # Generate a unique filename using a timestamp
            timestamp = time.strftime("%Y%m%d-%H%M%S")
            filename = os.path.join(DATA_FOLDER, f"capture_{timestamp}.jpg")
            
            try:
                # Get the raw binary data from the request body
                image_data = request.data
                
                # Save the image data to a file
                with open(filename, 'wb') as f:
                    f.write(image_data)
                
                print(f"SUCCESS: Image saved as {filename}")
                # Return a simple 200 OK response
                return jsonify({"status": "success", "filename": filename}), 200

            except Exception as e:
                print(f"ERROR: Failed to save image: {e}")
                return jsonify({"status": "error", "message": str(e)}), 500

        else:
            return jsonify({"status": "error", "message": "No image data received"}), 400

# --- Running the Server ---
if __name__ == '__main__':
    # Use your computer's local IP address if running on a different machine than the ESP32
    # Use 0.0.0.0 to listen on all interfaces, allowing external connections (like from the ESP32)
    app.run(host='0.0.0.0', port=5000, debug=True, threaded = True)