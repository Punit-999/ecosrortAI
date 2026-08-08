import tensorflow as tf
from tensorflow.keras.applications import MobileNetV2
from tensorflow.keras.layers import Dense, GlobalAveragePooling2D, Dropout
from tensorflow.keras.models import Model
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from sklearn.utils import class_weight
import numpy as np

IMG_SIZE = (224, 224)
BATCH_SIZE = 16
DATA_DIR = "./ecosort_split"

# --- Data generators with augmentation for training ---
train_datagen = ImageDataGenerator(
    rescale=1./255,
    rotation_range=30,
    zoom_range=0.25,
    width_shift_range=0.15,
    height_shift_range=0.15,
    horizontal_flip=True,
    brightness_range=[0.7, 1.3],
    shear_range=0.15
)
val_datagen = ImageDataGenerator(rescale=1./255)

train_gen = train_datagen.flow_from_directory(
    f"{DATA_DIR}/train", target_size=IMG_SIZE, batch_size=BATCH_SIZE,
    class_mode="binary"
)
val_gen = val_datagen.flow_from_directory(
    f"{DATA_DIR}/val", target_size=IMG_SIZE, batch_size=BATCH_SIZE,
    class_mode="binary"
)

print("Class indices:", train_gen.class_indices)
# This tells us which label is 0 and which is 1 - IMPORTANT, note this down

# --- Compute class weights to handle imbalance ---
weights = class_weight.compute_class_weight(
    class_weight="balanced",
    classes=np.unique(train_gen.classes),
    y=train_gen.classes
)
class_weights = {i: weights[i] for i in range(len(weights))}
print("Class weights:", class_weights)

# --- Build model using MobileNetV2 transfer learning ---
base_model = MobileNetV2(input_shape=(224, 224, 3), include_top=False, weights="imagenet")
base_model.trainable = False  # freeze base layers

x = base_model.output
x = GlobalAveragePooling2D()(x)
x = Dense(128, activation="relu")(x)
x = Dropout(0.3)(x)
output = Dense(1, activation="sigmoid")(x)

model = Model(inputs=base_model.input, outputs=output)
model.compile(optimizer="adam", loss="binary_crossentropy", metrics=["accuracy"])

model.summary()

# --- Train ---
history = model.fit(
    train_gen,
    validation_data=val_gen,
    epochs=15,
    class_weight=class_weights
)

model.save("ecosort_model.h5")
print("Model saved as ecosort_model.h5")