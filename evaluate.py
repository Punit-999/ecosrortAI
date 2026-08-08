import tensorflow as tf
from tensorflow.keras.models import load_model
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from sklearn.metrics import classification_report, confusion_matrix
import numpy as np

IMG_SIZE = (224, 224)
BATCH_SIZE = 16
DATA_DIR = "./ecosort_split"

model = load_model("ecosort_model.h5")

test_datagen = ImageDataGenerator(rescale=1./255)
test_gen = test_datagen.flow_from_directory(
    f"{DATA_DIR}/test", target_size=IMG_SIZE, batch_size=BATCH_SIZE,
    class_mode="binary", shuffle=False
)

loss, acc = model.evaluate(test_gen)
print(f"\nTest accuracy: {acc*100:.2f}%")

preds = model.predict(test_gen)
pred_labels = (preds > 0.5).astype(int).flatten()
true_labels = test_gen.classes

print("\nClassification Report:")
print(classification_report(true_labels, pred_labels, target_names=["non_recyclable", "recyclable"]))

print("\nConfusion Matrix:")
print(confusion_matrix(true_labels, pred_labels))