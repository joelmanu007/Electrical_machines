import streamlit as st
from ml_engine import InferenceEngine, get_diagnosis_color

# --- CONFIG ---
st.set_page_config(page_title="Air-Gapped Diagnostic Tool", page_icon="⚡", layout="wide")

# --- INITIALIZE ENGINE ---
@st.cache_resource
def load_engine():
    return InferenceEngine()

engine = load_engine()

# --- HEADER ---
st.title("⚡ Air-Gapped Diagnostic Tool")
st.markdown("Scan QR to pass URL params, or use sliders for manual input.")
st.divider()

# --- PARSE QUERY PARAMS ---
query_params = st.query_params

has_params = "temp" in query_params or "vib" in query_params or "curr" in query_params

if has_params:
    st.info("Using data from URL parameters.")
    
temp = float(query_params.get("temp", 60.0))
vib = float(query_params.get("vib", 3.0))
curr = float(query_params.get("curr", 5.0))

# --- SIDEBAR (MANUAL OVERRIDE) ---
with st.sidebar:
    st.header("🎛️ Manual Override")
    st.caption("Change values to test the diagnostic logic manually.")
    
    # If URL params drove the initial values, we still link them to sliders
    t_val = st.slider("Temperature (°C)", min_value=0.0, max_value=120.0, value=temp, step=1.0)
    v_val = st.slider("Vibration (mm/s)", min_value=0.0, max_value=20.0, value=vib, step=0.1)
    c_val = st.slider("Current (A)", min_value=0.0, max_value=30.0, value=curr, step=0.5)

# --- RUN INFERENCE ---
# Override with sidebar values
diagnosis = engine.predict(t_val, v_val, c_val)
diag_color = get_diagnosis_color(diagnosis)

# --- METRICS DASHBOARD ---
st.markdown("### 📊 Live Sensor Telemetry")
col1, col2, col3 = st.columns(3)

with col1:
    st.metric(label="Temperature", value=f"{t_val} °C")
    st.progress(min(t_val / 120.0, 1.0))
with col2:
    st.metric(label="Vibration", value=f"{v_val} mm/s")
    st.progress(min(v_val / 20.0, 1.0))
with col3:
    st.metric(label="Current", value=f"{c_val} A")
    st.progress(min(c_val / 30.0, 1.0))

st.divider()

# --- DIAGNOSIS ALERT BOX ---
st.markdown("### 🔍 Diagnosis Alert")

alert_html = f"""
<div style="background-color: {diag_color}; padding: 20px; border-radius: 10px; color: white; text-align: center; margin-top: 10px;">
    <h2 style="margin: 0; color: white;">Status: {diagnosis}</h2>
</div>
"""
st.markdown(alert_html, unsafe_allow_html=True)
