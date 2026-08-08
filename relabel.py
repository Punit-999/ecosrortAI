import os
import shutil

# Source path (using one consistent casing)
SOURCE = "./dataset/archive/Garbage classification/Garbage classification"

# Destination
DEST = "./ecosort_data"
os.makedirs(f"{DEST}/recyclable", exist_ok=True)
os.makedirs(f"{DEST}/non_recyclable", exist_ok=True)

RECYCLABLE_CLASSES = ["cardboard", "glass", "metal", "paper", "plastic"]
NON_RECYCLABLE_CLASSES = ["trash"]

def copy_images(class_list, dest_folder):
    count = 0
    for cls in class_list:
        src_path = os.path.join(SOURCE, cls)
        if not os.path.isdir(src_path):
            print(f"WARNING: {src_path} not found, skipping")
            continue
        for fname in os.listdir(src_path):
            src_file = os.path.join(src_path, fname)
            # prefix filename with class name to avoid collisions
            dst_file = os.path.join(DEST, dest_folder, f"{cls}_{fname}")
            shutil.copy2(src_file, dst_file)
            count += 1
    return count

recy_count = copy_images(RECYCLABLE_CLASSES, "recyclable")
non_recy_count = copy_images(NON_RECYCLABLE_CLASSES, "non_recyclable")

print(f"Copied {recy_count} images to recyclable/")
print(f"Copied {non_recy_count} images to non_recyclable/")