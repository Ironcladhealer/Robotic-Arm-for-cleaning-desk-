import cv2
import numpy as np
import os

# --- Configuration ---
# Path to the single reference image (the empty scene/background)
REFERENCE_IMAGE_PATH = 'data/reference_background.jpg' 
MIN_AREA_THRESHOLD = 300 # Minimum size of the object contour
DIFF_THRESHOLD = 30 # Pixel intensity threshold for binary conversion

class CVProcessor:
    def __init__(self):
        """Initializes by loading the reference background image."""
        self.reference_frame = None
        self.load_reference_image()

    def load_reference_image(self):
        """Loads the reference image in grayscale."""
        if not os.path.exists(REFERENCE_IMAGE_PATH):
            print(f"ERROR: Reference image not found at {REFERENCE_IMAGE_PATH}")
            self.reference_frame = None
            return

        # Load the image in grayscale (0)
        self.reference_frame = cv2.imread(REFERENCE_IMAGE_PATH, 0) 
        
        if self.reference_frame is not None:
            print("INFO: Reference background image loaded successfully.")
        else:
            print("ERROR: Failed to load reference image. Check file path and integrity.")

    def detect_trash(self, current_image_path):
        """
        Detects movement/new objects using background subtraction.
        Returns: True/False for detection success, and coordinates if found.
        """
        if self.reference_frame is None:
            print("ERROR: Cannot detect trash, reference frame is missing.")
            return False, None

        # 1. Load the current frame in grayscale
        frame = cv2.imread(current_image_path, 0)
        if frame is None:
             print(f"ERROR: Failed to load current image at {current_image_path}.")
             return False, None
        
        # Ensure frames are the same size before comparison
        if frame.shape != self.reference_frame.shape:
             print("ERROR: Reference and current frame sizes do not match.")
             return False, None
        
        # 2. Compute absolute difference between the reference and the new frame
        diff = cv2.absdiff(self.reference_frame, frame)
        
        # 3. Apply threshold to get a binary mask of the differences
        # Pixels with difference > DIFF_THRESHOLD become white (255)
        _, thresh = cv2.threshold(diff, DIFF_THRESHOLD, 255, cv2.THRESH_BINARY)
        
        # 4. Optional: Dilate the thresholded image to fill holes (improves contour finding)
        # thresh = cv2.dilate(thresh, None, iterations=2)

        # 5. Find contours (new objects/movement)
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for cnt in contours:
            area = cv2.contourArea(cnt)
            
            # Check if the object is large enough to be considered trash
            if area > MIN_AREA_THRESHOLD:
                x, y, w, h = cv2.boundingRect(cnt)
                
                # Trash detected!
                coordinates = {
                    "x_center": x + w / 2, 
                    "y_center": y + h / 2, 
                    "bounding_box": [x, y, x + w, y + h],
                    "area": area
                }
                
                print(f"DETECTED: Object found (Area: {area}) at center ({coordinates['x_center']:.2f}, {coordinates['y_center']:.2f})")
                return True, coordinates
            
        # No object large enough was found
        return False, None

# Initialize the processor immediately when the server starts
cv_processor = CVProcessor()