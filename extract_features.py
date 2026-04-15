import scipy.io
import numpy as np
import pandas as pd
import os

def calculate_features(chunk):
    """Calculates time-domain features for a given chunk of vibration data."""
    rms = np.sqrt(np.mean(chunk**2))
    peak = np.max(np.abs(chunk))
    crest_factor = peak / rms if rms > 0 else 0
    
    # Calculate Kurtosis manually to avoid extra dependencies
    mean = np.mean(chunk)
    std = np.std(chunk)
    kurtosis = np.mean(((chunk - mean) / std)**4) if std > 0 else 0
    
    return {
        'rms_vibration': rms,
        'peak_vibration': peak,
        'crest_factor': crest_factor,
        'kurtosis': kurtosis
    }

def process_mat_file(filepath, label, chunk_size=1024):
    """Reads a CWRU .mat file and extracts features into a list of dictionaries."""
    print(f"Processing: {filepath}...")
    mat_data = scipy.io.loadmat(filepath)
    
    # CWRU keys are usually formatted like 'X105_DE_time' (Drive End data)
    # We find the key that contains the Drive End ('DE') time series data
    de_key = [key for key in mat_data.keys() if 'DE_time' in key][0]
    raw_vibration = mat_data[de_key].flatten()
    
    features_list = []
    
    # Chop the continuous wave into discrete chunks
    for i in range(0, len(raw_vibration), chunk_size):
        chunk = raw_vibration[i:i + chunk_size]
        
        # Only process full-sized chunks
        if len(chunk) == chunk_size:
            features = calculate_features(chunk)
            features['label'] = label # 0 for Normal, 1 for Fault
            features_list.append(features)
            
    return features_list

def main():
    all_data = []
    
    # 1. Process Normal Data (Label: 0)
    # Make sure '97.mat' is in the same folder as this script
    if os.path.exists('97.mat'):
        normal_features = process_mat_file('97.mat', label=0)
        all_data.extend(normal_features)
    else:
        print("Error: Could not find 97.mat")

    # 2. Process Faulty Data (Label: 1)
    # Make sure '105.mat' is in the same folder
    if os.path.exists('105.mat'):
        fault_features = process_mat_file('105.mat', label=1)
        all_data.extend(fault_features)
    else:
        print("Error: Could not find 105.mat")

    # 3. Save to a clean CSV
    if all_data:
        df = pd.DataFrame(all_data)
        df.to_csv('motor_fault_data.csv', index=False)
        print("\nSuccess! Extracted", len(df), "samples.")
        print(df.head())

if __name__ == "__main__":
    main()