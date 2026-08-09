import cv2
import numpy as np
from tensorflow.keras.models import load_model

model = load_model("ecosort_model.h5")
cap = cv2.VideoCapture(0)

print("Press SPACE to scan, ESC to quit")

while True:
    ret, frame = cap.read()
    if not ret:
        break

    display_frame = frame.copy()
    cv2.putText(display_frame, "Press SPACE to scan", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    cv2.imshow("EcoSort AI - Live Scan", display_frame)

    key = cv2.waitKey(1) & 0xFF

    if key == 32:  # SPACE
        img = cv2.resize(frame, (224, 224))
        img = img.astype("float32") / 255.0
        img = np.expand_dims(img, axis=0)

        pred = model.predict(img, verbose=0)[0][0]

        if pred > 0.5:
            label = "RECYCLABLE"
            confidence = pred * 100
        else:
            label = "NON-RECYCLABLE"
            confidence = (1 - pred) * 100

        result_text = f"{label} ({confidence:.1f}%)"
        print(result_text)

        result_frame = frame.copy()
        color = (0, 255, 0) if label == "RECYCLABLE" else (0, 0, 255)
        cv2.putText(result_frame, result_text, (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, color, 3)
        cv2.imshow("EcoSort AI - Live Scan", result_frame)
        cv2.waitKey(2000)  # hold result on screen for 2 sec

    elif key == 27:  # ESC
        break

cap.release()
cv2.destroyAllWindows()
