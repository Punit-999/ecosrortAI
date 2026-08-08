# EcoSort AI

**An AI-powered smart waste sorting system for real-world deployment**

Built by **Team Surgeons of Tech** (ZEN-T370) — CODEAMBLE 2026 Grand Finale
Government Polytechnic Pune

---

## Overview

EcoSort AI is a camera-based waste classification and sorting system that identifies whether an item is recyclable or non-recyclable in real time, and physically diverts it into the correct compartment using a servo-driven mechanism. The system is designed as a retrofit module — a compact AI + sensor + diverter unit that can be fitted onto existing dustbins of different shapes and sizes, rather than requiring an entirely new bin infrastructure.

The project addresses a practical gap in India's waste management ecosystem: manual sorting at the point of disposal is inconsistent, and existing AI-based industrial sorting systems (AMP Robotics, TOMRA, Greyparrot) are built for large-scale material recovery facilities, not for households, housing societies, or municipal collection points.

---

## Problem Statement

Waste segregation at the source is one of the most effective ways to improve recycling rates, yet it remains largely dependent on manual human judgment, which is inconsistent and hard to enforce at scale. Existing AI-driven sorting solutions are expensive, industrial-scale systems not designed for household, society, or roadside deployment. EcoSort AI is built to close this gap with an affordable, adaptable, camera-based sorting module.

---

## Key Features

### 🎯 AI-Based Visual Classification
- A deep learning model built on **MobileNetV2** (transfer learning from ImageNet) classifies items as recyclable or non-recyclable directly from a live camera feed.
- Trained and iteratively improved across multiple rounds of real-world data collection, achieving **85% precision and 94% recall** on the non-recyclable class and **95%+ precision** on the recyclable class after refinement.
- Uses **class-weighted training** to correct for real-world data imbalance between recyclable and non-recyclable waste samples.

### 📷 Live Camera-Based Object Detection
- Real-time webcam integration with automatic object bounding-box detection, so only the item in frame — not the surrounding background — is passed to the classifier.
- Visual on-screen feedback showing detected object, classification result, and model confidence percentage.

### ⚙️ Mechanical Sorting Mechanism
- Servo-motor-driven diverter/flap mechanism physically routes classified items into the correct compartment.
- Arduino microcontroller acts as the bridge between the AI model's decision (via serial communication) and the physical sorting action.
- Prototype validated using a conveyor-belt-based sorting rig to demonstrate the full detect → decide → divert pipeline.

### 🧲 Secondary Sensor Confirmation
- **Inductive proximity sensor** for direct metal detection, used as a confirmation signal alongside the camera model — since vision alone is not always reliable for metal vs. non-metal distinction.
- **TCS3200 color/transparency sensor** explored as a secondary signal for distinguishing transparent recyclables (PET bottles, glass) from opaque organic waste, based on light transmission and reflectivity patterns.

### 🗑️ Adaptable Retrofit Design
EcoSort AI's core AI + sensor + diverter module is designed to fit multiple real-world bin form factors without redesigning the underlying system:
- **Household bucket bins** — compact retrofit with internal liner-bag compartments
- **Society/apartment bins** — wider chute, higher-throughput mechanism
- **Roadside municipal bins** — weatherproof housing with optional solar + battery backup
- **Novelty/character bins** (public spaces) — camera collar hidden within existing design, no external changes
- **Perforated steel office bins** — clip-on camera collar, no drilling required
- **Large wheeled municipal dumpsters** — reinforced motorized diverter for higher-volume waste

This "one core, many housings" approach is central to the project's real-world deployability, directly addressing scalability beyond a single fixed prototype shape.

### 📊 Rigorous, Transparent Model Development
The model was developed through an iterative, honest ML engineering process:
1. Initial training on a public garbage classification dataset (2,527 images)
2. Real-world testing exposed a critical gap — the model defaulted to "recyclable" for unfamiliar items like food waste and human hands
3. Custom data was captured directly (food scraps, peels, wrappers, tissue) and merged into the training set
4. A second, larger public dataset (30,000+ images, four-category waste taxonomy) was sampled, manually filtered for quality, and merged in a further refinement round
5. Each iteration was measured against a held-out test set using precision, recall, F1-score, and confusion matrices — not just overall accuracy — to ensure genuine improvement rather than superficial numbers

---

## System Architecture

```
Camera Input
      │
      ▼
AI Classification Model (MobileNetV2, on-device)
      │
      ▼
Sensor Confirmation Layer (Inductive proximity + Color/transparency sensor)
      │
      ▼
Microcontroller Decision Logic (Arduino)
      │
      ▼
Servo-Driven Mechanical Diverter
      │
      ▼
Correct Bin Compartment (Metal / Recyclable / Non-Recyclable)
```

---

## Tech Stack

| Layer | Technology |
|---|---|
| Machine Learning | TensorFlow / Keras, MobileNetV2 (transfer learning) |
| Computer Vision | OpenCV (webcam capture, object bounding-box detection) |
| Data Processing | Python, scikit-learn, Pillow |
| Hardware Control | Arduino (C++), Serial communication |
| Sensors | Inductive proximity sensor, TCS3200 color sensor |
| Actuation | Servo motors |
| Display | 16x2 character LCD (HD44780) |

---

## Model Performance

| Metric | Recyclable | Non-Recyclable |
|---|---|---|
| Precision | 95% | 85% |
| Recall | 88% | 94% |
| F1-Score | 91% | 89% |

*Final results after iterative dataset refinement across multiple training rounds, evaluated on a held-out test set of 688 images.*

---

## Project Structure

```
ecoml/
├── dataset/                  # Raw source dataset
├── ecosort_data/              # Relabeled binary dataset (recyclable / non_recyclable)
├── ecosort_split/              # Train / validation / test split
├── relabel.py                 # Converts multi-class dataset into binary labels
├── split_data.py               # Creates train/val/test split (70/15/15)
├── model.py                    # Builds and trains the MobileNetV2 classifier
├── evaluate_model.py            # Evaluates trained model on the test set
├── webcam_demo.py               # Live webcam classification demo
├── merge_new_dataset.py          # Samples and merges supplementary datasets
├── capture_images.py             # Utility for capturing custom training images
└── ecosort_model.h5               # Final trained model
```

---

## Future Scope

- Expand training data further to improve non-recyclable classification robustness on out-of-distribution items (e.g., large or unusual objects)
- Move from `.h5` to the native Keras format and optimize for on-device inference (Raspberry Pi / edge deployment)
- Integrate all sensor signals (camera, metal, color/transparency) into a unified confidence-weighted decision layer rather than independent checks
- Pilot deployment on a single real-world bin type to validate mechanical durability under continuous use
- Explore a companion mobile app for households/societies to track segregation compliance and environmental impact over time

---

## Team

**Surgeons of Tech** — ZEN-T370
Government Polytechnic Pune
CODEAMBLE 2026 Grand Finale — Smart and Mobility Domain

---

*EcoSort AI is a working demonstration of how affordable, camera-based AI classification combined with adaptable mechanical design can bring intelligent waste sorting to households, societies, and public spaces — not just industrial recycling facilities.*
