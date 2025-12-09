# backend/utils/cv_processor.py

import cv2
import numpy as np
import os

# --- Configuration ---
REFERENCE_IMAGE_PATH = 'data/reference_background.jpg' 
MIN_AREA_THRESHOLD = 300 # Minimum size of the object contour
DIFF_THRESHOLD = 30      # Pixel intensity threshold for binary conversion

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

        self.reference_frame = cv2.imread(REFERENCE_IMAGE_PATH, 0) 
        
        if self.reference_frame is not None:
            print("INFO: Reference background image loaded successfully.")
        else:
            print("ERROR: Failed to load reference image. Check file path and integrity.")

    def detect_trash(self, current_image_path):
        """
        Detects movement/new objects using background subtraction.
        Returns: True/False for detection success, and coordinates/size if found.
        """
        if self.reference_frame is None:
            return False, None

        frame = cv2.imread(current_image_path, 0)
        if frame is None or frame.shape != self.reference_frame.shape:
             return False, None
        
        # Background Subtraction
        diff = cv2.absdiff(self.reference_frame, frame)
        _, thresh = cv2.threshold(diff, DIFF_THRESHOLD, 255, cv2.THRESH_BINARY)
        
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for cnt in contours:
            area = cv2.contourArea(cnt)
            
            if area > MIN_AREA_THRESHOLD:
                x, y, w, h = cv2.boundingRect(cnt)
                
                # Return the necessary information for distance calculation and IK
                coordinates = {
                    "x_center_px": x + w / 2, 
                    "y_center_px": y + h / 2, 
                    "width_pixels": w,          # <--- Used for distance estimation
                    "height_pixels": h,
                    "area": area,
                    "bounding_box": [x, y, x + w, y + h]
                }
                
                print(f"DETECTED: Object found (Area: {area})")
                return True, coordinates
            
        return False, None

# Initialize the processor immediately when the server starts
cv_processor = CVProcessor()