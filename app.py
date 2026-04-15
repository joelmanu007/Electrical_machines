import streamlit as st
import pandas as pd
import joblib
import plotly.graph_objects as go

# --- PAGE CONFIG ---
st.set_page_config(page_title="Motor Digital Twin", page_icon="⚙️", layout="wide")

# --- MODEL LOADING ---
@st.cache_resource
def load_model():
    return joblib.load("real_motor_model.pkl")

try:
    model = load_model()
except FileNotFoundError:
    st.error("Model not found! Ensure 'real_motor_model.pkl' is in the directory.")
    st.stop()

# --- HELPER FUNCTION: GAUGE CHARTS ---
def create_gauge(value, title, max_val, danger_threshold):
    fig = go.Figure(go.Indicator(
        mode = "gauge+number",
        value = value,
        title = {'text': title, 'font': {'size': 20}},
        gauge = {
            'axis': {'range': [None, max_val]},
            'bar': {'color': "darkblue"},
            'steps': [
                {'range': [0, danger_threshold * 0.7], 'color': "lightgreen"},
                {'range': [danger_threshold * 0.7, danger_threshold], 'color': "gold"},
                {'range': [danger_threshold, max_val], 'color': "salmon"}],
            'threshold': {
                'line': {'color': "red", 'width': 4},
                'thickness': 0.75,
                'value': danger_threshold}
        }))
    fig.update_layout(height=250, margin=dict(l=10, r=10, t=40, b=10))
    return fig

# --- HEADER ---
st.title("⚙️ Induction Motor: Real-Time Diagnostic Twin")
st.markdown("Air-Gapped Telemetry & AI Inference Engine")
st.divider()

# --- DATA INGESTION (QR URL OR SLIDERS) ---
query_params = st.query_params
def get_param(key, default):
    return float(query_params[key]) if key in query_params else default

# Sidebar for Manual Override/Testing
with st.sidebar:
    st.header("🎛️ Manual Override")
    st.caption("Use sliders if no QR data is present.")
    rms = st.slider("RMS Vibration", 0.0, 1.0, get_param("rms", 0.12), 0.01)
    peak = st.slider("Peak Vibration", 0.0, 2.0, get_param("peak", 0.35), 0.01)
    crest = st.slider("Crest Factor", 0.0, 10.0, get_param("crest", 2.9), 0.1)
    kurt = st.slider("Kurtosis", 0.0, 15.0, get_param("kurtosis", 3.1), 0.1)

# --- AI INFERENCE ---
input_data = pd.DataFrame({'rms_vibration': [rms], 'peak_vibration': [peak], 'crest_factor': [crest], 'kurtosis': [kurt]})
prediction = model.predict(input_data)[0]

# --- UI: AI VERDICT BANNER ---
if prediction == 0:
    st.success("✅ **SYSTEM HEALTHY:** All vibration harmonics are within normal operational limits.")
elif prediction == 1:
    st.error("🚨 **CRITICAL FAULT DETECTED:** High-frequency bearing anomalies present. Maintenance required immediately.")
else:
    st.warning("⚠️ **ANOMALY:** Unclassified vibration signature.")

st.divider()

# --- UI: TELEMETRY GAUGES ---
st.markdown("### 📊 Edge Sensor Telemetry")
col1, col2, col3, col4 = st.columns(4)

with col1:
    st.plotly_chart(create_gauge(rms, "RMS (Continuous Energy)", 1.0, 0.5), use_container_width=True)
with col2:
    st.plotly_chart(create_gauge(peak, "Peak (Max Impact)", 2.0, 1.2), use_container_width=True)
with col3:
    st.plotly_chart(create_gauge(crest, "Crest Factor", 10.0, 5.0), use_container_width=True)
with col4:
    st.plotly_chart(create_gauge(kurt, "Kurtosis (Spikiness)", 15.0, 5.0), use_container_width=True)

# --- UI: SYSTEM DETAILS EXPANDER ---
with st.expander("🛠️ View Raw Model Inputs & System Details"):
    st.dataframe(input_data, use_container_width=True)
    st.caption("Inference provided by Random Forest Classifier (n_estimators=100)")