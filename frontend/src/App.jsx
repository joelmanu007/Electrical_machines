import React, { useState, useCallback, useEffect, useRef } from 'react';

const MAX_HISTORY = 50;

export default function App() {
  // ── IOT WEB FETCH ARCHITECTURE ──────────────────────────────
  // The ESP32 is now pushing data directly to the backend. We just fetch it!
  const [rms, setRms] = useState(0.0);
  const [peak, setPeak] = useState(0.0);
  const [crest, setCrest] = useState(0.0);
  const [kurtosis, setKurtosis] = useState(0.0);
  const [status, setStatus] = useState("WAITING_FOR_SENSOR");
  const [confidence, setConfidence] = useState(0.0);
  const [isSyncing, setIsSyncing] = useState(true);
  const [activeTab, setActiveTab] = useState("dashboard");
  const [scanHistory, setScanHistory] = useState([]);
  const [showSettings, setShowSettings] = useState(false);
  const [decommissionMsg, setDecommissionMsg] = useState(null);

  const fetchLatestData = useCallback(async () => {
    setIsSyncing(true);
    try {
      const response = await fetch(`${import.meta.env.VITE_API_URL}/api/latest`);
      if (response.ok) {
        const data = await response.json();
        
        // Update dashboard state
        setRms(data.rms);
        setPeak(data.peak);
        setCrest(data.crest_factor);
        setKurtosis(data.kurtosis);
        setStatus(data.status);
        if (data.confidence !== undefined) setConfidence(data.confidence);

        // Append to local history
        setScanHistory(prev => {
          // Prevent exact duplicate history logging using a simple check
          if (prev.length > 0 && prev[0].rms === data.rms && prev[0].status === data.status) {
             return prev;
          }
          return [{
            timestamp: new Date().toISOString(),
            rms: data.rms,
            peak: data.peak,
            crest_factor: data.crest_factor,
            kurtosis: data.kurtosis,
            status: data.status,
            confidence: data.confidence,
          }, ...prev].slice(0, MAX_HISTORY);
        });
      }
    } catch (error) {
      console.error("Failed to fetch latest sensor data:", error);
    } finally {
      setIsSyncing(false);
    }
  }, []);

  // Poll for new data every 3 seconds
  useEffect(() => {
    fetchLatestData(); // initial fetch
    const interval = setInterval(fetchLatestData, 3000);
    return () => clearInterval(interval);
  }, [fetchLatestData]);

  const handleDecommission = () => {
    const msg = `Unit IND-4492-B scheduled for decommission at ${new Date().toLocaleString()}. Maintenance team notified.`;
    setDecommissionMsg(msg);
    setActiveTab("alerts");
  };

  const faultAlerts = scanHistory.filter(s => s.status !== "HEALTHY");
  const getDashOffset = (val, max) => 251.2 * (1 - Math.min(val, max) / max);

  return (
    <div className="bg-background text-on-background font-body selection:bg-primary/30 min-h-screen">

      {/* ───── SETTINGS MODAL ───── */}
      {showSettings && (
        <div
          className="fixed inset-0 z-[100] flex items-center justify-center bg-black/70 backdrop-blur-sm"
          onClick={() => setShowSettings(false)}
        >
          <div
            className="bg-slate-900 border border-slate-700 p-8 max-w-md w-full mx-4 shadow-2xl"
            onClick={e => e.stopPropagation()}
          >
            <div className="flex justify-between items-center mb-6">
              <h3 className="font-headline text-lg font-bold text-white tracking-tight">System Settings</h3>
              <button className="text-slate-400 hover:text-white transition-colors" onClick={() => setShowSettings(false)}>
                <span className="material-symbols-outlined">close</span>
              </button>
            </div>
            <div className="space-y-4 text-xs font-mono">
              <div className="bg-slate-800 p-4 space-y-1">
                <p className="text-cyan-400 font-bold uppercase mb-2">Model Configuration</p>
                <p className="text-slate-300">Type: <span className="text-white">Random Forest Classifier</span></p>
                <p className="text-slate-300">Estimators: <span className="text-white">100</span></p>
                <p className="text-slate-300">Features: <span className="text-white">rms, peak, crest_factor, kurtosis</span></p>
                <p className="text-slate-300">Training Data: <span className="text-white">CWRU Bearing Dataset (97.mat / 105.mat)</span></p>
              </div>
              <div className="bg-slate-800 p-4 space-y-1">
                <p className="text-cyan-400 font-bold uppercase mb-2">API Configuration</p>
                <p className="text-slate-300">Backend: <span className="text-white break-all">{import.meta.env.VITE_API_URL}</span></p>
                <p className="text-slate-300">Endpoint: <span className="text-white">/api/diagnose</span></p>
              </div>
              <div className="bg-slate-800 p-4 space-y-1">
                <p className="text-cyan-400 font-bold uppercase mb-2">Session Stats</p>
                <p className="text-slate-300">Total Scans: <span className="text-white">{scanHistory.length}</span></p>
                <p className="text-slate-300">Faults Detected: <span className="text-error">{faultAlerts.length}</span></p>
                <p className="text-slate-300">Healthy Scans: <span className="text-emerald-400">{scanHistory.length - faultAlerts.length}</span></p>
              </div>
            </div>
            <button
              className="mt-6 w-full py-2 text-xs font-bold tracking-widest uppercase border border-cyan-400/30 text-cyan-400 hover:bg-cyan-400/10 transition-all"
              onClick={() => setShowSettings(false)}
            >
              Close
            </button>
          </div>
        </div>
      )}

      {/* ───── TOP APP BAR ───── */}
      <header className="bg-slate-950/80 backdrop-blur-xl top-0 sticky z-50 shadow-[0_4px_30px_rgba(0,0,0,0.5)]">
        <div className="flex justify-between items-center px-6 py-3 w-full">
          <div className="flex items-center gap-4">
            <span className="font-headline tracking-tighter uppercase text-xl font-bold text-cyan-400 drop-shadow-[0_0_8px_rgba(60,215,255,0.5)]">
              MOTOR_DIAGNOSTIC_v4.2
            </span>
            <div className="h-4 w-px bg-slate-700"></div>
            <div className="hidden sm:flex flex-col">
              <span className="text-[10px] text-slate-500 uppercase tracking-widest font-bold">Last Scanned</span>
              <span className="text-xs text-cyan-400 font-mono">
                {scanHistory.length > 0 ? new Date(scanHistory[0].timestamp).toLocaleString() : "Never"}
              </span>
            </div>
          </div>
          <div className="flex items-center gap-6">
            <nav className="hidden md:flex gap-6">
              {["Dashboard", "History", "Alerts"].map(tab => (
                <button
                  key={tab}
                  onClick={() => setActiveTab(tab.toLowerCase())}
                  className={`py-1 font-headline tracking-tighter uppercase text-sm transition-colors relative ${
                    activeTab === tab.toLowerCase()
                      ? "text-cyan-400 border-b-2 border-cyan-400"
                      : "text-slate-400 hover:text-cyan-300"
                  }`}
                >
                  {tab}
                  {tab === "Alerts" && faultAlerts.length > 0 && (
                    <span className="ml-1 px-1.5 py-0.5 bg-red-600 text-white text-[8px] font-black rounded-full">
                      {faultAlerts.length}
                    </span>
                  )}
                </button>
              ))}
            </nav>
            <div className="flex items-center gap-2">
              <button
                onClick={runDiagnosis}
                disabled={isSyncing}
                title="Quick Sync"
                className="p-2 hover:bg-slate-800 transition-colors cursor-pointer active:scale-95 rounded text-cyan-400 disabled:opacity-40"
              >
                <span className={`material-symbols-outlined ${isSyncing ? "animate-spin" : ""}`} style={{ fontVariationSettings: "'FILL' 0" }}>sync_alt</span>
              </button>
              <button
                onClick={() => setShowSettings(true)}
                title="Settings"
                className="p-2 hover:bg-slate-800 transition-colors cursor-pointer active:scale-95 rounded text-cyan-400"
              >
                <span className="material-symbols-outlined" style={{ fontVariationSettings: "'FILL' 0" }}>settings</span>
              </button>
            </div>
          </div>
        </div>
      </header>

      <div className="flex">
        {/* ───── SIDE NAV ───── */}
        <aside className="h-screen w-64 fixed left-0 top-0 bg-slate-900 flex flex-col pt-20 pb-8 z-40 hidden lg:flex">
          <div className="px-6 mb-8">
            <h2 className="text-lg font-black text-white font-headline">KINETIC COMMAND</h2>
            <p className="text-[10px] text-cyan-400 font-mono tracking-tighter">SYSTEM_STABLE</p>
          </div>
          <nav className="flex-1 space-y-1">
            {[
              { id: "dashboard", icon: "analytics", label: "Dashboard" },
              { id: "history",   icon: "history",   label: "History" },
              { id: "alerts",    icon: "warning",    label: `Alerts${faultAlerts.length > 0 ? ` (${faultAlerts.length})` : ""}` },
            ].map(item => (
              <button
                key={item.id}
                onClick={() => setActiveTab(item.id)}
                className={`w-full flex items-center gap-3 px-6 py-4 font-body text-xs font-semibold uppercase tracking-widest transition-all active:translate-x-1 ${
                  activeTab === item.id
                    ? "bg-cyan-400/10 text-cyan-400 border-r-4 border-cyan-400"
                    : "text-slate-500 hover:bg-slate-800/30 hover:text-cyan-200"
                }`}
              >
                <span className="material-symbols-outlined">{item.icon}</span>
                {item.label}
              </button>
            ))}
          </nav>
          <div className="px-6 mb-6">
            <button
              onClick={runDiagnosis}
              disabled={isSyncing}
              className="w-full py-3 bg-cyan-400/10 border border-cyan-400/30 text-cyan-400 font-headline text-xs font-bold tracking-widest hover:bg-cyan-400 hover:text-slate-950 transition-all disabled:opacity-50"
            >
              {isSyncing ? "SCANNING..." : "INITIATE SCAN"}
            </button>
          </div>
        </aside>

        {/* ───── MAIN CONTENT ───── */}
        <main className="flex-1 lg:ml-64 p-6 lg:p-10 bg-slate-950 min-h-screen pb-24 lg:pb-10">
          <div className="max-w-7xl mx-auto space-y-8">

            {/* ════════════════════════ DASHBOARD ════════════════════════ */}
            {activeTab === "dashboard" && (
              <section className="grid grid-cols-1 xl:grid-cols-4 gap-6">
                <div className="xl:col-span-3 space-y-8">

                  {/* QR Scan Detection Banner */}
                  {hasQRParams && (
                    <div className="p-3 bg-cyan-950/60 border border-cyan-400/40 flex items-center gap-3">
                      <span className="material-symbols-outlined text-cyan-400 text-base" style={{ fontVariationSettings: "'FILL' 1" }}>qr_code_scanner</span>
                      <div>
                        <p className="text-cyan-300 text-xs font-mono font-bold tracking-widest uppercase">QR Scan Detected</p>
                        <p className="text-cyan-500 text-[10px] font-mono mt-0.5">Sensor data loaded from ESP32. AI diagnosis running automatically...</p>
                      </div>
                    </div>
                  )}

                  {/* Verdict Banner */}
                  {status === "CRITICAL_FAULT" && (
                    <div className="relative overflow-hidden p-6 bg-red-950/60 border-l-8 border-red-500 flex items-center gap-6 shadow-[0_20px_50px_rgba(147,0,10,0.3)]">
                      <div className="flex-shrink-0 w-16 h-16 rounded-full bg-red-600 flex items-center justify-center animate-pulse">
                        <span className="material-symbols-outlined text-white text-3xl" style={{ fontVariationSettings: "'FILL' 1" }}>warning</span>
                      </div>
                      <div className="flex-1">
                        <h3 className="font-headline font-bold text-2xl text-red-200 tracking-tight">CRITICAL: INNER RACE BEARING FAULT</h3>
                        <p className="text-red-300/80 text-sm font-medium mt-1">Structural degradation detected in drive-end bearing assembly. Immediate intervention required.</p>
                        <p className="text-red-400/60 text-xs font-mono mt-1">AI CONFIDENCE: {confidence}%</p>
                      </div>
                      <button
                        onClick={handleDecommission}
                        className="flex-shrink-0 px-6 py-2 bg-red-200 text-red-900 font-bold text-xs tracking-widest uppercase hover:bg-white transition-all"
                      >
                        DECOMMISSION
                      </button>
                      <div className="absolute top-0 right-0 p-2 opacity-10 pointer-events-none">
                        <span className="material-symbols-outlined text-9xl">dangerous</span>
                      </div>
                    </div>
                  )}

                  {status === "HEALTHY" && (
                    <div className="relative overflow-hidden p-6 bg-emerald-950/60 border-l-8 border-emerald-500 flex items-center gap-6 shadow-[0_20px_50px_rgba(16,185,129,0.2)]">
                      <div className="flex-shrink-0 w-16 h-16 rounded-full bg-emerald-500 flex items-center justify-center">
                        <span className="material-symbols-outlined text-emerald-950 text-3xl" style={{ fontVariationSettings: "'FILL' 1" }}>check_circle</span>
                      </div>
                      <div className="flex-1">
                        <h3 className="font-headline font-bold text-2xl text-emerald-400 tracking-tight">SYSTEM OPTIMAL: NORMAL OPERATION</h3>
                        <p className="text-emerald-400/80 text-sm font-medium mt-1">Kinetic readouts are nominal. No anomalies detected in bearing assembly.</p>
                        <p className="text-emerald-400/50 text-xs font-mono mt-1">AI CONFIDENCE: {confidence}%</p>
                      </div>
                    </div>
                  )}

                  {status === "CONNECTION_ERROR" && (
                    <div className="p-6 bg-slate-800/50 border-l-8 border-slate-600 flex items-center gap-6">
                      <span className="material-symbols-outlined text-slate-400 text-4xl">wifi_off</span>
                      <div>
                        <h3 className="font-headline font-bold text-xl text-slate-300">BACKEND OFFLINE</h3>
                        <p className="text-slate-400 text-sm mt-1">Cannot reach FastAPI server. Ensure <code className="text-cyan-400">uvicorn main:app --reload</code> is running.</p>
                      </div>
                    </div>
                  )}

                  {/* Gauges */}
                  <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
                    {[
                      { label: "Velocity RMS", val: rms, max: 2, unit: "mm/s", color: "text-sky-400" },
                      { label: "Peak Velocity", val: peak, max: 5, unit: "mm/s", color: "text-cyan-400" },
                      { label: "Crest Factor", val: crest, max: 10, unit: "RATIO", color: "text-cyan-400" },
                      { label: "Kurtosis", val: kurtosis, max: 10, unit: kurtosis > 5 ? "FAULT ▲" : "NORMAL", color: kurtosis > 5 ? "text-red-400" : "text-cyan-400" },
                    ].map(({ label, val, max, unit, color }) => (
                      <div key={label} className={`bg-slate-800/60 p-6 flex flex-col items-center justify-between min-h-[220px] transition-all ${label === "Kurtosis" && val > 5 ? "ring-2 ring-red-500/50" : ""}`}>
                        <span className={`text-[10px] font-headline font-bold tracking-[0.2em] uppercase self-start w-full text-left ${color}`}>{label}</span>
                        <div className="relative flex items-center justify-center py-4 flex-1">
                          <svg className="w-24 h-24 transform -rotate-90">
                            <circle className="text-slate-700" cx="48" cy="48" fill="transparent" r="40" stroke="currentColor" strokeWidth="4" />
                            <circle className={color + " transition-all duration-700 ease-in-out"} cx="48" cy="48" fill="transparent" r="40" stroke="currentColor" strokeDasharray="251.2" strokeDashoffset={getDashOffset(val, max)} strokeWidth="8" />
                          </svg>
                          <div className="absolute inset-0 flex flex-col items-center justify-center">
                            <span className={`text-2xl font-mono font-bold ${color}`}>{parseFloat(val).toFixed(2)}</span>
                            <span className="text-[8px] text-slate-500 font-bold mt-0.5">{unit}</span>
                          </div>
                        </div>
                        <div className="w-full h-1 bg-slate-700 mt-auto overflow-hidden">
                          <div className={`h-full transition-all duration-700 ${color.replace("text-", "bg-")}`} style={{ width: `${Math.min((val / max) * 100, 100)}%` }} />
                        </div>
                      </div>
                    ))}
                  </div>

                  {/* JSON Readout */}
                  <div className="bg-slate-900 overflow-hidden border border-slate-800">
                    <div className="bg-slate-800/60 px-6 py-3 flex justify-between items-center">
                      <h4 className="font-headline text-[10px] font-bold uppercase tracking-widest text-slate-400 flex items-center gap-2">
                        <span className="material-symbols-outlined text-sm">terminal</span> Raw Telemetry JSON
                      </h4>
                      <div className="flex gap-2 items-center">
                        <div className="w-2 h-2 rounded-full bg-red-500 animate-pulse" />
                        <span className="text-[9px] text-slate-500 font-mono uppercase">LIVE</span>
                      </div>
                    </div>
                    <div className="p-6 font-mono text-[11px] leading-relaxed text-cyan-300/70 bg-slate-950 h-52 overflow-y-auto">
                      <pre>{JSON.stringify({
                        timestamp: new Date().toISOString(),
                        motor_id: "IND-4492-B",
                        sensor_array: "X-AXIS_VIB_01",
                        sampling_rate: 25600,
                        metrics: {
                          rms: parseFloat(rms),
                          peak: parseFloat(peak),
                          crest_factor: parseFloat(crest),
                          kurtosis: parseFloat(kurtosis),
                        },
                        verdict: status,
                        confidence_pct: confidence,
                      }, null, 2)}</pre>
                    </div>
                  </div>
                </div>

                {/* Right Sidebar */}
                <div className="space-y-6">
                  {/* Simulator */}
                  <div className="bg-slate-800/60 p-6">
                    <h4 className="font-headline text-[10px] font-bold uppercase tracking-widest text-slate-400 mb-6 border-b border-slate-700 pb-2">Hardware Simulator</h4>
                    <div className="space-y-8">
                      {[
                        { label: "RMS Velocity", val: rms, set: setRms, min: 0, max: 2, step: 0.01 },
                        { label: "Peak Value",   val: peak, set: setPeak, min: 0, max: 5, step: 0.01 },
                        { label: "Crest Factor", val: crest, set: setCrest, min: 1, max: 10, step: 0.1 },
                        { label: "Kurtosis",     val: kurtosis, set: setKurtosis, min: 1, max: 10, step: 0.1, warn: kurtosis > 5 },
                      ].map(({ label, val, set, min, max, step, warn }) => (
                        <div key={label} className="space-y-3">
                          <div className="flex justify-between items-end">
                            <label className={`text-[10px] font-bold uppercase tracking-wider ${warn ? "text-red-400" : "text-slate-400"}`}>{label}</label>
                            <span className={`font-mono text-xs font-bold ${warn ? "text-red-400" : "text-cyan-400"}`}>{parseFloat(val).toFixed(2)}</span>
                          </div>
                          <input
                            className={`w-full appearance-none h-1 rounded ${warn ? "accent-red-500" : "accent-cyan-400"}`}
                            type="range" min={min} max={max} step={step} value={val}
                            onChange={e => set(e.target.value)}
                          />
                        </div>
                      ))}
                      <button
                        onClick={runDiagnosis}
                        disabled={isSyncing}
                        className={`w-full py-4 font-headline text-xs font-black tracking-[0.2em] uppercase transition-all text-slate-950 ${
                          isSyncing ? "bg-slate-600 text-slate-300" : "bg-cyan-400 hover:bg-cyan-300 hover:shadow-[0_0_20px_rgba(34,211,238,0.4)]"
                        }`}
                      >
                        {isSyncing ? "SYNCING..." : "SYNC VIRTUAL TWIN"}
                      </button>
                    </div>
                  </div>

                  {/* Live Node Status */}
                  <div className="bg-slate-800/60 p-6 space-y-4">
                    <h4 className="font-headline text-[10px] font-bold uppercase tracking-widest text-slate-400 mb-2">Live Node Status</h4>
                    {[
                      {
                        label: "ML Backend",
                        sub: status !== "CONNECTION_ERROR" ? "ONLINE" : "OFFLINE",
                        color: status !== "CONNECTION_ERROR" ? "bg-cyan-400" : "bg-slate-600",
                      },
                      {
                        label: "Motor Status",
                        sub: status,
                        color: status === "HEALTHY" ? "bg-emerald-400" : status === "CRITICAL_FAULT" ? "bg-red-500 animate-pulse" : "bg-slate-600",
                      },
                      {
                        label: "Session Scans",
                        sub: `${scanHistory.length} total / ${faultAlerts.length} faults`,
                        color: "bg-cyan-400",
                      },
                    ].map(({ label, sub, color }) => (
                      <div key={label} className="flex items-center gap-4">
                        <div className={`w-3 h-3 rounded-sm flex-shrink-0 ${color}`} />
                        <div>
                          <p className="text-[10px] font-bold text-white uppercase tracking-tighter">{label}</p>
                          <p className="text-[8px] text-slate-500 font-mono">{sub}</p>
                        </div>
                      </div>
                    ))}
                  </div>

                  {/* Twin Image */}
                  <div className="relative group bg-slate-900 overflow-hidden border border-slate-800">
                    <img
                      alt="Industrial motor"
                      className="w-full h-40 object-cover opacity-60 grayscale group-hover:grayscale-0 group-hover:scale-110 transition-all duration-700"
                      src="https://lh3.googleusercontent.com/aida-public/AB6AXuDmlyLqmk7GREP9DvH5GibfeVeM3o6TvHWhZetmthVSbzoXoA68hccDdzCaNws1Ex3KTmSL2zmjuQlv58Od9TSVP93Z4t9z6NEzh2BTvG-uIt6Ra-VdrsTw5Xypd-_JcR0gHP2J7G2ZmEHIZ7RMFXwEA8R15sd61f6cNphrq9PPSz9vEHfbYheIf2EB6BeWnoMJcp28R5m-4C_DSX_o5SvAAh50YaoAw61dBjgT6_N-uuLi0v-cLJ5hve8o8J2EHIdZ-WrIDATM4A"
                    />
                    <div className="absolute inset-0 bg-gradient-to-t from-slate-950 via-slate-950/20 to-transparent" />
                    <div className="absolute bottom-4 left-4">
                      <span className="px-2 py-1 bg-cyan-400 text-slate-950 text-[8px] font-black tracking-widest uppercase">Twin View</span>
                      <p className="text-xs font-headline font-bold text-white mt-2">UNIT_ID: IND-4492-B</p>
                    </div>
                  </div>
                </div>
              </section>
            )}

            {/* ════════════════════════ HISTORY TAB ════════════════════════ */}
            {activeTab === "history" && (
              <section className="space-y-6">
                <div className="flex items-center justify-between">
                  <div>
                    <h2 className="font-headline text-2xl font-bold text-white tracking-tight">Scan History</h2>
                    <p className="text-slate-500 text-xs mt-1 font-mono">{scanHistory.length} of {MAX_HISTORY} entries stored in session</p>
                  </div>
                  {scanHistory.length > 0 && (
                    <button
                      onClick={() => setScanHistory([])}
                      className="text-[10px] font-bold text-slate-500 hover:text-red-400 uppercase tracking-widest transition-colors border border-slate-700 hover:border-red-500/50 px-4 py-2"
                    >
                      Clear History
                    </button>
                  )}
                </div>

                {scanHistory.length === 0 ? (
                  <div className="flex flex-col items-center justify-center py-40 text-slate-700">
                    <span className="material-symbols-outlined text-6xl mb-4">history</span>
                    <p className="font-headline text-sm uppercase tracking-widest">No scans recorded yet</p>
                    <p className="text-xs mt-2">Go to Dashboard and click <span className="text-cyan-500">SYNC VIRTUAL TWIN</span></p>
                  </div>
                ) : (
                  <div className="bg-slate-900 overflow-x-auto border border-slate-800">
                    <table className="w-full text-xs font-mono">
                      <thead className="bg-slate-800/80">
                        <tr className="text-[10px] text-slate-400 uppercase tracking-widest">
                          <th className="px-5 py-3 text-left">#</th>
                          <th className="px-5 py-3 text-left">Timestamp</th>
                          <th className="px-4 py-3 text-right">RMS</th>
                          <th className="px-4 py-3 text-right">Peak</th>
                          <th className="px-4 py-3 text-right">Crest</th>
                          <th className="px-4 py-3 text-right">Kurtosis</th>
                          <th className="px-5 py-3 text-center">Status</th>
                          <th className="px-4 py-3 text-right">Confidence</th>
                        </tr>
                      </thead>
                      <tbody>
                        {scanHistory.map((scan, i) => (
                          <tr
                            key={i}
                            className={`border-t border-slate-800 hover:bg-slate-800/30 transition-colors ${
                              scan.status !== "HEALTHY" ? "bg-red-950/20" : ""
                            }`}
                          >
                            <td className="px-5 py-3 text-slate-600">{scanHistory.length - i}</td>
                            <td className="px-5 py-3 text-slate-300 whitespace-nowrap">{new Date(scan.timestamp).toLocaleString()}</td>
                            <td className="px-4 py-3 text-right text-sky-300">{scan.rms.toFixed(3)}</td>
                            <td className="px-4 py-3 text-right text-cyan-300">{scan.peak.toFixed(3)}</td>
                            <td className="px-4 py-3 text-right text-cyan-300">{scan.crest_factor.toFixed(3)}</td>
                            <td className={`px-4 py-3 text-right font-bold ${scan.kurtosis > 5 ? "text-red-400" : "text-cyan-300"}`}>
                              {scan.kurtosis.toFixed(3)}
                            </td>
                            <td className="px-5 py-3 text-center">
                              <span className={`px-2 py-1 text-[9px] font-black uppercase tracking-widest ${
                                scan.status === "HEALTHY"
                                  ? "bg-emerald-900/60 text-emerald-400"
                                  : "bg-red-900/60 text-red-400"
                              }`}>
                                {scan.status}
                              </span>
                            </td>
                            <td className="px-4 py-3 text-right text-slate-300">{scan.confidence}%</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </section>
            )}

            {/* ════════════════════════ ALERTS TAB ════════════════════════ */}
            {activeTab === "alerts" && (
              <section className="space-y-6">
                <div className="flex items-center gap-4">
                  <h2 className="font-headline text-2xl font-bold text-white tracking-tight">Fault Alerts</h2>
                  {faultAlerts.length > 0 && (
                    <span className="px-2 py-1 bg-red-600 text-white text-xs font-black rounded">{faultAlerts.length}</span>
                  )}
                </div>

                {decommissionMsg && (
                  <div className="p-4 bg-emerald-950/60 border border-emerald-500/40 flex items-center gap-3">
                    <span className="material-symbols-outlined text-emerald-400">check_circle</span>
                    <p className="text-emerald-400 text-sm font-medium">{decommissionMsg}</p>
                    <button onClick={() => setDecommissionMsg(null)} className="ml-auto text-emerald-600 hover:text-emerald-400">
                      <span className="material-symbols-outlined text-sm">close</span>
                    </button>
                  </div>
                )}

                {faultAlerts.length === 0 ? (
                  <div className="flex flex-col items-center justify-center py-40 text-slate-700">
                    <span className="material-symbols-outlined text-6xl mb-4 text-emerald-800">check_circle</span>
                    <p className="font-headline text-sm uppercase tracking-widest text-emerald-700">No faults detected</p>
                    <p className="text-xs mt-2">All scans have returned healthy results.</p>
                  </div>
                ) : (
                  <div className="space-y-4">
                    {faultAlerts.map((alert, i) => (
                      <div key={i} className="p-5 bg-red-950/40 border-l-4 border-red-500 flex items-start gap-5">
                        <span className="material-symbols-outlined text-red-400 text-3xl mt-0.5 flex-shrink-0" style={{ fontVariationSettings: "'FILL' 1" }}>warning</span>
                        <div className="flex-1 min-w-0">
                          <p className="font-headline font-bold text-red-200 text-sm uppercase tracking-wide">
                            {alert.status.replace(/_/g, " ")}
                          </p>
                          <p className="text-red-400/60 text-xs font-mono mt-1">
                            {new Date(alert.timestamp).toLocaleString()}
                          </p>
                          <div className="flex flex-wrap gap-4 mt-2 text-[10px] font-mono text-slate-400">
                            <span>RMS: <span className="text-sky-300">{alert.rms.toFixed(3)}</span></span>
                            <span>Peak: <span className="text-cyan-300">{alert.peak.toFixed(3)}</span></span>
                            <span>Crest: <span className="text-cyan-300">{alert.crest_factor.toFixed(3)}</span></span>
                            <span>Kurtosis: <span className="text-red-400 font-bold">{alert.kurtosis.toFixed(3)}</span></span>
                            <span>Confidence: <span className="text-white">{alert.confidence}%</span></span>
                          </div>
                        </div>
                      </div>
                    ))}
                  </div>
                )}
              </section>
            )}

          </div>
        </main>
      </div>

      {/* ───── MOBILE BOTTOM NAV ───── */}
      <nav className="lg:hidden fixed bottom-0 left-0 w-full bg-slate-950/95 backdrop-blur-xl flex justify-around items-center p-3 z-50 border-t border-slate-800">
        {[
          { id: "dashboard", icon: "analytics", label: "Dash" },
          { id: "history",   icon: "history",   label: "History" },
          { id: "alerts",    icon: "warning",    label: "Alerts" },
        ].map(item => (
          <button
            key={item.id}
            onClick={() => setActiveTab(item.id)}
            className={`flex flex-col items-center transition-colors ${activeTab === item.id ? "text-cyan-400" : "text-slate-500"}`}
          >
            <span className="material-symbols-outlined" style={activeTab === item.id ? { fontVariationSettings: "'FILL' 1" } : {}}>{item.icon}</span>
            <span className="text-[8px] font-bold uppercase mt-1">{item.label}</span>
          </button>
        ))}
        <button
          onClick={() => setShowSettings(true)}
          className="flex flex-col items-center text-slate-500 transition-colors"
        >
          <span className="material-symbols-outlined">settings</span>
          <span className="text-[8px] font-bold uppercase mt-1">Setup</span>
        </button>
      </nav>
    </div>
  );
}
