import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report, accuracy_score
import joblib

print("Loading data...")
# 1. Load your extracted features
try:
    df = pd.read_csv("motor_fault_data.csv")
except FileNotFoundError:
    print("Error: Could not find motor_fault_data.csv. Did you run extract_features.py?")
    exit()

# 2. Define Features (Inputs) and Labels (Outputs)
# We drop the 'label' column to isolate our inputs
X = df[['rms_vibration', 'peak_vibration', 'crest_factor', 'kurtosis']]
y = df['label'] # 0 is Normal, 1 is Faulty

# 3. Split the data into Training and Testing sets
# We hold back 20% of the data to test the AI on data it hasn't seen before
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print(f"Training on {len(X_train)} samples, testing on {len(X_test)} samples.")

# 4. Initialize and Train the Model
print("Training the Random Forest model...")
model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

# 5. Evaluate the Model
print("\n--- Model Evaluation ---")
predictions = model.predict(X_test)
accuracy = accuracy_score(y_test, predictions)
print(f"Overall Accuracy: {accuracy * 100:.2f}%\n")
print("Detailed Report:")
print(classification_report(y_test, predictions, target_names=['Normal (0)', 'Fault (1)']))

# 6. Export the "Brain"
print("Saving model to real_motor_model.pkl...")
joblib.dump(model, "real_motor_model.pkl")
print("Done! Your AI is ready for the web dashboard.")