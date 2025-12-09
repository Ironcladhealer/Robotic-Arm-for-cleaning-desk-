# Robotic Arm for Cleaning Desk

Small project combining an ESP32-CAM and a robotic arm to detect and pick up small trash items (example: red ball / crumpled paper) using OpenCV and a Flask backend.

## Features
- Background-subtraction and red-object detection utilities
- Distance estimation using a simple pinhole camera model
- Utilities exposed in `backend/utils` for image processing and distance estimation
- Example functions to process single images (no webcam required)

## Repo layout (relevant)
- backend/
  - utils/
    - cv_processor.py    -- background subtraction / trash detection
    - distance.py        -- red-object mask, detection and distance estimation
  - (Flask server and other backend code)

## Requirements
- Python 3.8+
- OpenCV (opencv-python)
- numpy

Example install (Windows, from project root):
```powershell
python -m venv .venv
.venv\Scripts\activate
python -m pip install --upgrade pip
python -m pip install opencv-python numpy
```

If pip launcher fails, use `python -m pip install ...` as shown above.

## Usage examples

- Detect a red ball in a single image (example using distance utility):
```python
from backend.utils.distance import process_image

ok, res = process_image(image_path="path\\to\\image.jpg")
if ok:
    print("x_center (px):", res["x_center"])
    print("y_center (px):", res["y_center"])
    print("bounding_box (px):", res["bounding_box"])
    print("distance (cm):", res["distance_cm"])
else:
    print("No red object detected")
```

- Background-subtraction (cv_processor):
```python
from backend.utils.cv_processor import cv_processor

found, info = cv_processor.detect_trash("path\\to\\current_image.jpg")
if found:
    print(info)  # contains x_center_px, y_center_px, width_pixels, height_pixels, area, bounding_box
```

## Calibration & Tuning
- distance.py uses a pinhole model: Z = (W * F) / P
  - KNOWN_TRASH_WIDTH_CM and FOCAL_LENGTH_PIXELS must be set for accurate distance estimates.
  - Current distance output field is `distance_cm` (centimetres). Convert to metres: `m = distance_cm / 100.0`.
- Red detection may need tuning for lighting and camera:
  - Adjust HSV thresholds and morphological settings in `backend/utils/distance.py -> get_red_mask`.
  - Lower `min_area` in `process_image` to detect smaller objects.
  - Save debug mask with `cv2.imwrite("debug_mask.png", mask)` to inspect what is being detected.

## Troubleshooting
- "No red object detected" while object is visible:
  - Check image colors (BGR vs grayscale) and lighting; try more inclusive HSV ranges in `get_red_mask`.
  - Reduce `min_area` or relax roundness constraint in `find_red_object`.
  - Verify image path and that `cv2.imread` returns a valid image.
- Pip launcher errors on Windows:
  - Use `python -m pip ...` or recreate the virtual environment.

## Notes
- x_center / y_center and bounding box coordinates are in pixels (not SI).
- Distance results are in centimetres by default.
- This README is minimal; extend with server run instructions once Flask entrypoint is known.