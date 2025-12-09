import numpy as np
import math
from numpy import degrees, arccos, arctan2, clip

# --- Robot Dimensions (USE YOUR MEASURED VALUES) ---
L2 = 14.3      # Length of the second link (Shoulder to Elbow) in cm
L3 = 9.4       # Length of the third link (Elbow to Wrist) in cm

# --- Fixed Target Height ---
# H: Vertical distance from the Shoulder Pivot down to the paper on the ground.
# If the paper is below the pivot, H is negative. MUST BE MEASURED.
TARGET_VERTICAL_REACH_CM = -5.0 # Example: 5cm below the shoulder pivot

# --- Calibration Constants ---
IMAGE_CENTER_PX = 160 # For QVGA (320x240), center X is 320/2
MAX_PAN_ANGLE_CHANGE = 50 # Max deviation from center (e.g., 90 +/- 50 = 40 to 140 deg range)

# --- Servo Offset Constants (CRITICAL FOR FIXING NEGATIVE ANGLES) ---
# These are the measured angles when the arm is physically STRAIGHT OUT (IK Angle = 0 or 180)
# YOU MUST FINE-TUNE THESE. 
SHOULDER_ZERO_OFFSET = 90  # Servo 2 is at 90 deg when the arm is horizontal
ELBOW_ZERO_OFFSET = 180    # Servo 3 adjustment for 180 deg max reach
SHOULDER_MIN_ANGLE = 20    # Safety clamp: avoid hitting the base
SHOULDER_MAX_ANGLE = 160


def calculate_ik_angles(coords_2d_pixels, target_distance_cm):
    """
    Calculates the 4 servo angles based on the 2D pixel position and the 3D distance.
    
    Returns:
        dict: A dictionary of 4 servo angles (s1 to s4) or None if unreachable.
    """
    
    target_x_center_px = coords_2d_pixels['x_center_px']
    
    # --- 1. Angle 1 (Base Rotation/Pan - Servo 1) ---
    pixel_offset = target_x_center_px - IMAGE_CENTER_PX 
    angle_1 = 90 + (pixel_offset / IMAGE_CENTER_PX) * MAX_PAN_ANGLE_CHANGE
    s1_base = int(clip(angle_1, 0, 180)) 
    
    
    # --- 2. 2D IK for Shoulder & Elbow (Servo 2 & 3) ---
    R = target_distance_cm # Horizontal reach from base
    H = TARGET_VERTICAL_REACH_CM # Vertical height relative to shoulder pivot
    
    C_sq = R**2 + H**2
    C = math.sqrt(C_sq)
    
    # Unreachable check: If C is too long or too short
    if C > (L2 + L3) or C < abs(L2 - L3):
        print(f"ERROR: Target (R={R:.1f}cm, H={H:.1f}cm) is unreachable.")
        return None 
    
    # Calculate angles using Law of Cosines
    
    # Angle Beta (internal angle at elbow joint)
    cos_beta = (L2**2 + L3**2 - C_sq) / (2 * L2 * L3)
    beta = arccos(clip(cos_beta, -1.0, 1.0))
    
    # Angle Gamma (angle at the shoulder formed by L2 and line C)
    cos_gamma = (L2**2 + C_sq - L3**2) / (2 * L2 * C)
    gamma = arccos(clip(cos_gamma, -1.0, 1.0))
    
    # Angle Alpha (angle of line C relative to the horizontal R axis)
    alpha = arctan2(H, R)
    
    # IK Angle 2 (angle L2 makes with the horizontal)
    theta2_rad = alpha + gamma
    
    # IK Angle 3 (angle L3 makes with L2)
    theta3_rad = math.pi - beta
    
    
    # --- 3. Map IK Angles to Physical Servo Angles (0-180) ---
    
    # S3 Elbow Angle (X-axis movement)
    # The physical servo angle often corresponds directly to the internal angle (180 - beta)
    s3_elbow_x = degrees(theta3_rad) 
    
    # S2 Shoulder Angle (Z-axis movement)
    # The calculated IK angle theta2 is absolute. We must adjust it by an offset 
    # (e.g., 90 degrees) to align with the servo's physical mounting.
    s2_shoulder_z = degrees(theta2_rad) + SHOULDER_ZERO_OFFSET 
    
    
    # --- 4. Clamping and Final Assembly ---
    
    s2_shoulder_z = int(clip(s2_shoulder_z, SHOULDER_MIN_ANGLE, SHOULDER_MAX_ANGLE))
    s3_elbow_x = int(clip(s3_elbow_x, 0, 180)) # Elbow is usually a 0-180 range
    
    # Angle 4 (Gripper): Open
    s4_gripper_open = 10 
    
    # Return the angles
    return {
        "s1_base": s1_base, 
        "s2_shoulder_z": s2_shoulder_z, 
        "s3_elbow_x": s3_elbow_x, 
        "s4_gripper_open": s4_gripper_open
    }