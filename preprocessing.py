import pandas as pd
import numpy as np
import os
import glob


def load_and_preprocess(data_dir, frame_size=100, overlap=0.5):
    """
    Loading all csv logs, differentiates, normalizes and segmentation operations.
    Re-Naming: node_A_env_forest.csv
    """
    frames_all = []
    labels_env = []
    labels_node = []

    csv_files = glob.glob(os.path.join(data_dir, "*.csv"))

    for filepath in csv_files:
        filename = os.path.basename(filepath)
        # node_A_env_forest.csv → node=A, env=forest
        parts = filename.replace(".csv", "").split("_")
        node_id = parts[1]  # the node id e.g. "A"
        env_id = parts[3]  # the environment e.g. "forest"

        df = pd.read_csv(filepath, names=["timestamp", "rssi", "lqi"])
        rssi = df["rssi"].values.astype(float)

        # 1. Differentiation, so focus will be on the change in the link quality alone
        y = np.diff(rssi)

        # 2. Normalization to [0, 1], to prevent from bias
        y_min, y_max = y.min(), y.max()
        if y_max - y_min == 0:
            continue
        z = (y - y_min) / (y_max - y_min)

        # 3. Segmentation from time series into model inputs with Overlap
        step = int(frame_size * (1 - overlap))
        for start in range(0, len(z) - frame_size, step):
            frame = z[start:start + frame_size]
            frames_all.append(frame)
            labels_env.append(env_id)
            labels_node.append(node_id)

    X = np.array(frames_all, dtype=np.float32)
    return X, np.array(labels_env), np.array(labels_node)
