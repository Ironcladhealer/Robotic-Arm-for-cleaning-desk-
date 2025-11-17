from flask import Flask, request, jsonify
import os

app = Flask(__name__)

UPLOAD_FOLDER = "static/uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/detect', methods=['POST'])
def detect():
    if 'image' not in request.files:
        return jsonify({'error': 'No image file found'}), 400

    image = request.files['image']
    image_path = os.path.join(UPLOAD_FOLDER, image.filename)
    image.save(image_path)
    print(f"[INFO] Received image: {image.filename}")

    return jsonify({'message': f'Image {image.filename} saved successfully'}), 200

if __name__ == "__main__":
    app.run(debug=True)
