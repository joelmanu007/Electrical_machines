from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import joblib
import pandas as pd

# Global dictionary to hold machine learning models
ml_models = {}

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Load the saved ML model on startup
    try:
        ml_models["model"] = joblib.load("real_motor_model.pkl")
        feature_names = ml_models["model"].feature_names_in_.tolist()
        classes = ml_models["model"].classes_.tolist()
        print(f"✅ Model loaded successfully.")
        print(f"   Feature names: {feature_names}")
        print(f"   Classes: {classes}")
    except Exception as e:
        print(f"❌ Warning: Failed to load real_motor_model.pkl: {e}")
        ml_models["model"] = None
    yield
    ml_models.clear()

app = FastAPI(title="Digital Twin Motor Diagnostic API", lifespan=lifespan)

# CORS middleware — allow all origins so the React frontend can call us locally
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Pydantic model for request validation
class DiagnoseRequest(BaseModel):
    rms: float
    peak: float
    crest_factor: float
    kurtosis: float

@app.get("/")
async def root():
    return {"message": "Motor Diagnostic API is running.", "model_loaded": ml_models.get("model") is not None}

@app.post("/api/diagnose")
async def diagnose(request: DiagnoseRequest):
    model = ml_models.get("model")
    if model is None:
        raise HTTPException(status_code=503, detail="Motor model is not available. Ensure real_motor_model.pkl exists.")

    try:
        # CRITICAL: Column names must match EXACTLY what the model was trained with.
        # Trained with: rms_vibration, peak_vibration, crest_factor, kurtosis
        input_data = pd.DataFrame([{
            "rms_vibration": request.rms,
            "peak_vibration": request.peak,
            "crest_factor": request.crest_factor,
            "kurtosis": request.kurtosis
        }])

        # Run inference
        prediction_val = int(model.predict(input_data)[0])

        # Model was trained on 2 classes: 0 = Normal, 1 = Fault
        # We map these to UI status strings
        status_map = {
            0: "HEALTHY",
            1: "CRITICAL_FAULT"
        }

        status = status_map.get(prediction_val, "CRITICAL_FAULT")

        # Also get prediction probabilities for richer data
        proba = model.predict_proba(input_data)[0].tolist()

        return {
            "prediction": prediction_val,
            "status": status,
            "confidence": round(max(proba) * 100, 1),
            "probabilities": {
                "healthy": round(proba[0] * 100, 1),
                "fault": round(proba[1] * 100, 1)
            }
        }

    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Inference error: {str(e)}")
