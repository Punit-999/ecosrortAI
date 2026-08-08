import os
import shutil
import random

random.seed(42)  # reproducibility

SOURCE = "./ecosort_data"
DEST = "./ecosort_split"
CLASSES = ["recyclable", "non_recyclable"]
SPLIT_RATIOS = {"train": 0.70, "val": 0.15, "test": 0.15}

for split in SPLIT_RATIOS:
    for cls in CLASSES:
        os.makedirs(f"{DEST}/{split}/{cls}", exist_ok=True)

for cls in CLASSES:
    src_folder = os.path.join(SOURCE, cls)
    files = os.listdir(src_folder)
    random.shuffle(files)

    n = len(files)
    n_train = int(n * SPLIT_RATIOS["train"])
    n_val = int(n * SPLIT_RATIOS["val"])

    train_files = files[:n_train]
    val_files = files[n_train:n_train + n_val]
    test_files = files[n_train + n_val:]

    for fname in train_files:
        shutil.copy2(os.path.join(src_folder, fname), f"{DEST}/train/{cls}/{fname}")
    for fname in val_files:
        shutil.copy2(os.path.join(src_folder, fname), f"{DEST}/val/{cls}/{fname}")
    for fname in test_files:
        shutil.copy2(os.path.join(src_folder, fname), f"{DEST}/test/{cls}/{fname}")

    print(f"{cls}: {len(train_files)} train, {len(val_files)} val, {len(test_files)} test")