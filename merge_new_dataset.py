import os
import random
import shutil

random.seed(42)

BASE = "./archive"
DEST = "./ecosort_data"

# Map: (relative path from BASE) -> target class
SOURCES = {
    # Recyclable subcategories
    "Recyclable/Recyclable/plastic_bottles": "recyclable",
    "Recyclable/Recyclable/glass_containers": "recyclable",
    "Recyclable/Recyclable/cans_all_type": "recyclable",
    "Recyclable/Recyclable/paper_products": "recyclable",

    # Organic -> non_recyclable (wet waste)
    "Organic/Organic/food_scraps": "non_recyclable",
    "Organic/Organic/coffee_tea_bags": "non_recyclable",
    "Organic/Organic/yard_trimmings": "non_recyclable",
    "Organic/Organic/egg_shells": "non_recyclable",
    "Organic/Organic/kitchen_waste": "non_recyclable",

    # Non-Recyclable -> non_recyclable
    "Non-Recyclable/Non-Recyclable/stroform_product": "non_recyclable",
    "Non-Recyclable/Non-Recyclable/diapers": "non_recyclable",
    "Non-Recyclable/Non-Recyclable/ceramic_product": "non_recyclable",
    "Non-Recyclable/Non-Recyclable/sanitary_napkin": "non_recyclable",
    "Non-Recyclable/Non-Recyclable/platics_bags_wrappers": "non_recyclable",

    # Hazardous -> non_recyclable
    "Hazardous/Hazardous/batteries": "non_recyclable",
    "Hazardous/Hazardous/pesticides": "non_recyclable",
    "Hazardous/Hazardous/paints": "non_recyclable",
    "Hazardous/Hazardous/e-waste": "non_recyclable",
}

# How many images to sample per subfolder
SAMPLES_PER_FOLDER = {
    "recyclable": 60,       # you already have plenty, just adding variety
    "non_recyclable": 120,  # priority - your weak class
}

os.makedirs(f"{DEST}/recyclable", exist_ok=True)
os.makedirs(f"{DEST}/non_recyclable", exist_ok=True)

total_counts = {"recyclable": 0, "non_recyclable": 0}

for rel_path, target_class in SOURCES.items():
    src_folder = os.path.join(BASE, rel_path)
    if not os.path.isdir(src_folder):
        print(f"WARNING: {src_folder} not found, skipping")
        continue

    files = os.listdir(src_folder)
    n_sample = min(SAMPLES_PER_FOLDER[target_class], len(files))
    sampled_files = random.sample(files, n_sample)

    subfolder_name = os.path.basename(rel_path)

    for fname in sampled_files:
        src_file = os.path.join(src_folder, fname)
        dst_file = os.path.join(DEST, target_class, f"new2_{subfolder_name}_{fname}")
        shutil.copy2(src_file, dst_file)
        total_counts[target_class] += 1

    print(f"{rel_path}: sampled {n_sample}/{len(files)} -> {target_class}")

print(f"\nTotal added -> recyclable: {total_counts['recyclable']}, non_recyclable: {total_counts['non_recyclable']}")