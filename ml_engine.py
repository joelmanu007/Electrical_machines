import random
import time

class InferenceEngine:
    def __init__(self):
        # In a real scenario, we would load a trained model here (e.g. scikit-learn random forest)
        # self.model = joblib.load('my_model.pkl')
        pass

    def predict(self, temp: float, vib: float, curr: float) -> str:
        """
        Dummy prediction logic based on thresholds.
        Status: 'Normal', 'Overheating Risk', 'Bearing Wear', or 'Stator Overload'
        """
        # Simulate slight inference delay
        time.sleep(0.5)

        # Basic threshold rules for demonstration purposes
        if temp > 85.0:
            return "Overheating Risk"
        elif vib > 8.0:
            return "Bearing Wear"
        elif curr > 12.0:
            return "Stator Overload"
        else:
            return "Normal"

def get_diagnosis_color(status: str) -> str:
    if status == "Normal":
        return "#28a745" # Green
    elif status == "Overheating Risk":
        return "#dc3545" # Red
    elif status == "Bearing Wear":
        return "#fd7e14" # Orange
    elif status == "Stator Overload":
        return "#ffc107" # Yellow
    return "#6c757d" # Gray
