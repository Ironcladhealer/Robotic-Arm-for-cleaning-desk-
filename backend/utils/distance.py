# backend/utils/distance.py

import numpy as np

# --- Calibration Constants (MEASURE/CALCULATE THESE) ---
# W: Real-world width of the crumpled paper (e.g., 5cm) - MUST BE AN ASSUMED VALUE
KNOWN_TRASH_WIDTH_CM = 5.0 
# F: Focal length of the ESP32-CAM in pixels (Calculated during calibration)
FOCAL_LENGTH_PIXELS = 13.6 


def estimate_distance(width_pixels):
    """
    Estimates distance using the Pinhole Camera Model (Z = (F * W) / P).
    
    Args:
        width_pixels (int): The width of the object's bounding box in pixels (P).
        
    Returns:
        float: Distance to the object in centimeters (Z).
    """
    if width_pixels == 0:
        return None
    
    # Z = (W * F) / P
    distance = (KNOWN_TRASH_WIDTH_CM * FOCAL_LENGTH_PIXELS) / width_pixels
    return distance