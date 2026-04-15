# Design System Specification: Industrial Precision & Telemetric Depth

## 1. Overview & Creative North Star
The Creative North Star for this design system is **"The Kinetic Command Center."** 

This is not a generic administrative dashboard; it is a high-stakes, high-precision environment where telemetry meets editorial elegance. We are moving away from the "web-template" aesthetic to embrace an **Industrial Brutalist** philosophy. The layout should feel like a physical piece of aerospace hardware: dense with information yet perfectly legible, utilizing intentional asymmetry to guide the eye toward critical data points. We prioritize functional depth over flat decoration, using layers of slate and neon to simulate a multi-focal cockpit experience.

---

## 2. Colors: Tonal Immersion
Our palette is rooted in the "Deep Slate" spectrum, using neon accents not as decoration, but as functional status indicators.

### The Color Roles
- **Primary (`#a8e8ff` / `#3cd7ff`):** Electric Blue. Used for active states, data streams, and "Go" signals.
- **Secondary (`#ffb77d` / `#fd8b00`):** Safety Orange. Reserved for warnings, standby states, and caution-level telemetry.
- **Tertiary/Error (`#ffb4ab` / `#93000a`):** Danger Red. Used exclusively for critical system failures and immediate-action triggers.
- **Neutrals (`#111316` to `#333538`):** Deep Slate Grays. These provide the structural foundation of the control room environment.

### The "No-Line" Rule
**Explicit Instruction:** Prohibit 1px solid borders for sectioning. Structural boundaries must be defined solely through background color shifts. For example:
- A `surface-container-low` component sits directly on a `surface` background.
- Use `surface-container-highest` to draw the eye to a specific panel without ever drawing a line around it.

### Surface Hierarchy & Nesting
Treat the UI as a series of physical layers. Use the surface-container tiers to create "nested" depth:
1. **Base Level (`surface`):** The main "desk" or floor of the dashboard.
2. **Level 2 (`surface-container-low`):** Secondary modules or navigation rails.
3. **Level 3 (`surface-container-high`):** Primary interactive panels.
4. **Level 4 (`surface-container-highest`):** Active pop-overs or critical telemetry cards.

### The "Glass & Glow" Rule
Floating elements (modals, tooltips, or detached navigation) must utilize **Glassmorphism**. Use semi-transparent surface colors with a `backdrop-blur` (12px–20px). Main CTAs should use a subtle linear gradient from `primary` to `primary_container` to provide a "lit from within" neon effect that flat colors cannot achieve.

---

## 3. Typography: Editorial Utility
The typography system juxtaposes the human-centric **Space Grotesk** with the machine-centric **Inter** (simulating Akkurat Mono/LayGrotesk qualities).

- **Display & Headlines (Space Grotesk):** These are your "Editorial Markers." Use large scales (`display-lg`) for system status or high-level KPIs. The wide, geometric stance of Space Grotesk feels engineered and authoritative.
- **Body & Titles (Inter):** High-readability sans-serif for descriptions and labels.
- **Telemetry & Data (Doto-Variable / Monospace Fallback):** For all numeric values, timestamps, and sensor readouts. The monospace nature ensures that fluctuating numbers don't cause layout "jitter," maintaining the feeling of a steady, calibrated instrument.

---

## 4. Elevation & Depth: Tonal Layering
We reject traditional drop shadows. Depth is achieved through light and material density.

- **The Layering Principle:** Stacking is the new shadowing. Place a `surface-container-lowest` card on a `surface-container-low` section to create a soft "recessed" look.
- **Ambient Glows:** When a "floating" effect is required, shadows must be extra-diffused (40px-64px blur) and low-opacity (6%). The shadow should be tinted with a hint of `primary` or `surface_tint` to mimic the ambient light cast by a high-tech screen.
- **The "Ghost Border" Fallback:** If accessibility requires a container edge, use a "Ghost Border": the `outline_variant` token at **15% opacity**. Never use a 100% opaque border.

---

## 5. Components: The Industrial Primitive

### Buttons
- **Primary:** High-glow neon (`primary` to `primary_container` gradient). Sharp corners (`0.25rem`).
- **Secondary:** Transparent background with a "Ghost Border." Text in `on_surface`.
- **States:** On hover, primary buttons should increase their "glow" (outer shadow opacity increases), not just change color.

### Telemetry Chips
- Used for status labels (e.g., "STABLE", "OVERHEAT").
- Use `tertiary_container` for high-alert chips. No borders; use high-contrast text (`on_tertiary_container`) against the muted background.

### Input Fields
- **Styling:** Minimalist. Only a bottom "underline" using the `outline` token or a subtle `surface-container-highest` background fill.
- **Focus:** The bottom line transitions to `primary` (Electric Blue) with a soft outer glow.

### Monitoring Cards
- **Forbid dividers.** Separate content sections within a card using vertical whitespace (16px/24px) or a tonal shift (e.g., a `surface-container-highest` header area on a `surface-container-high` card).

### System Terminal (Additional Component)
- A dedicated area for log readouts. Use `surface_container_lowest` for the background, 100% monospace typography, and `primary_fixed_dim` for the text color to simulate a legacy CRT phosphor glow.

---

## 6. Do's and Don'ts

### Do:
- **Asymmetric Balance:** Place a large KPI on the left and three smaller telemetry streams on the right. 
- **Monospace for Numbers:** Always. Precision is key.
- **Breathing Room:** Give telemetry data space to "breathe" so operators can process information during high-stress alerts.

### Don't:
- **No Rounded Corners > 8px:** This is an industrial tool, not a social media app. Keep the `roundedness` scale strictly to `sm` (2px) and `DEFAULT` (4px) for most components.
- **No 1px Borders:** Never use a solid line to separate sections. Use color shifts.
- **No Pure Black/White:** Use the `surface` and `on_surface` tokens to maintain the "Slate" atmosphere and reduce eye strain in low-light control room environments.