import cv2
import os

SAVE_DIR = "./new_recyclable"
os.makedirs(SAVE_DIR, exist_ok=True)

cap = cv2.VideoCapture(0)
count = 0

print("Press SPACE to capture, ESC to quit")
print(f"Saving images to {SAVE_DIR}")

while True:
    ret, frame = cap.read()
    if not ret:
        break

    display = frame.copy()
    cv2.putText(display, f"Captured: {count}  (SPACE=capture, ESC=quit)",
                (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    cv2.imshow("Capture non-recyclable items", display)

    key = cv2.waitKey(1) & 0xFF
    if key == 32:  # SPACE
        fname = os.path.join(SAVE_DIR, f"custom_recy_{count+30}.jpg")
        cv2.imwrite(fname, frame)
        count += 1
        print(f"Saved {fname}")
    elif key == 27:  # ESC
        break

cap.release()
cv2.destroyAllWindows()
print(f"Total captured: {count}")