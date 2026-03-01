import torch
from torch.utils.data import Dataset
from sklearn.preprocessing import LabelEncoder

class RSSIDataset(Dataset):
    def __init__(self, X, labels):
        self.le = LabelEncoder()
        self.y = torch.tensor(self.le.fit_transform(labels), dtype=torch.long)
        # Shape: [N, 1, frame_size] → 1D Conv erwartet (batch, channels, length)
        self.X = torch.tensor(X, dtype=torch.float32).unsqueeze(1)
        self.classes = self.le.classes_

    def __len__(self):
        return len(self.X)

    def __getitem__(self, idx):
        return self.X[idx], self.y[idx]