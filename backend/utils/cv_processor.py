# backend/utils/cv_processor.py

from ultralytics import YOLO
import numpy as np

# --- Configuration ---
# Replace 'yolov8n.pt' with your specific trash model path if you have one!
MODEL_PATH = 'yolov8n.pt'
CONFIDENCE_THRESHOLD = 0.5
TARGET_CLASS_ID = 0 # Placeholder for 'paper' or 'trash' class index

class CVProcessor:
    def __init__(self):
        """Initializes and loads the YOLO model."""
        self.model = None
        self.load_model()

    def load_model(self):
        """Loads the YOLO model using the ultralytics library."""
        try:
            print(f"INFO: Loading YOLO model from {MODEL_PATH}...")
            # Load the model weights
            self.model = YOLO(MODEL_PATH) 
            print("INFO: YOLO model loaded successfully.")
        except Exception as e:
            print(f"ERROR: Failed to load YOLO model: {e}")
            self.model = None

    def detect_trash(self, image_path):
        """
        Runs YOLO detection on an image.
        Returns: True/False for detection success, and a dictionary of coordinates.
        """
        if not self.model:
            print("ERROR: Model is not available for detection.")
            return False, None

        # Run inference on the image
        # verbose=False suppresses the console output from YOLO
        results = self.model(image_path, verbose=False)

        for r in results:
            boxes = r.boxes
            if len(boxes) > 0:
                # Iterate through all detections
                for i in range(len(boxes)):
                    confidence = boxes.conf[i].item()
                    cls = int(boxes.cls[i].item())
                    box = boxes.xyxy[i].tolist() # [x_min, y_min, x_max, y_max]

                    # Check if it meets the criteria (confidence > threshold AND correct class)
                    # NOTE: For now, we accept any object with high confidence as trash.
                    if confidence >= CONFIDENCE_THRESHOLD: # and cls == TARGET_CLASS_ID:
                        
                        # Calculate the center coordinates for the robotic arm
                        x_center = (box[0] + box[2]) / 2
                        y_center = (box[1] + box[3]) / 2
                        
                        coordinates = {
                            "x_center": x_center, 
                            "y_center": y_center, 
                            "bounding_box": box,
                            "confidence": confidence
                        }
                        
                        print(f"DETECTED: Class {cls}, Conf: {confidence:.2f}")
                        return True, coordinates
            
        return False, None

# Initialize the processor immediately when the server starts
cv_processor = CVProcessor()