// VuMeter.jsx — Analog VU meter matching the physical reference meter:
//   • Non-linear ANSI VU scale (−20 to +3 VU)
//   • Two rows of scale numbers: outer (large, outside arc) + inner (small, dimmer, inside arc)
//   • Meter-type label ("VU" / "RMS" / "K-14" etc.) centered on dial face — NOT at top
//   • Thick red zone from 0 → +3 VU (5px full / 3px compact)
//   • Spring-physics needle + orange peak-hold needle
//
// Export: VuMeter
// Props: value (dB, -20..+3), peakHold (dB|null), label (string),
//        width, height, compact (bool), simulate (bool)

const { useRef, useEffect } = React;

// Non-linear ANSI VU scale positions (matches VuMeter.h kScale[])
const VU_SCALE = [
  { v: -20, t: 0.000 }, { v: -10, t: 0.218 }, { v:  -7, t: 0.324 },
  { v:  -5, t: 0.410 }, { v:  -3, t: 0.508 }, { v:  -2, t: 0.570 },
  { v:  -1, t: 0.636 }, { v:   0, t: 0.710 }, { v:  +1, t: 0.793 },
  { v:  +2, t: 0.870 }, { v:  +3, t: 1.000 },
];

function vuToT(v) {
  const c = Math.max(-20, Math.min(3, v));
  for (let i = 0; i < VU_SCALE.length - 1; i++) {
    const a = VU_SCALE[i], b = VU_SCALE[i + 1];
    if (c >= a.v && c <= b.v) return a.t + (c - a.v) / (b.v - a.v) * (b.t - a.t);
  }
  return c <= -20 ? 0 : 1;
}

// Arc extent (clockwise from 12 o'clock, radians)
const A0 = -0.92, A1 = 0.92;
const ang = t => A0 + t * (A1 - A0);
const T0  = VU_SCALE[7].t; // t at 0 VU

function VuMeter({
  value    = -12,
  peakHold = null,
  label    = 'VU',
  width    = 300,
  height   = 180,
  compact  = false,
  simulate = false,
}) {
  const canvasRef = useRef(null);
  const stateRef  = useRef({ needle: value, vel: 0, peak: peakHold, peakTimer: 0 });
  const animRef   = useRef(null);
  const targetRef = useRef(value);
  const labelRef  = useRef(label);

  useEffect(() => { targetRef.current = value; }, [value]);
  useEffect(() => { labelRef.current  = label; }, [label]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;

    // Pivot is at bottom-centre of the arc area, slightly above canvas bottom.
    const cx  = W / 2;
    const cy  = Math.round(H * 0.94);
    const R   = compact ? Math.round(W * 0.36) : Math.round(W * 0.378);
    const dpr = 2; // canvas is 2× for HiDPI

    function draw(needleVal, holdVal) {
      ctx.clearRect(0, 0, W, H);

      // ── Arcs ───────────────────────────────────────────────────────────────
      // Black: −20 → 0 VU
      ctx.lineWidth   = compact ? 1.5 * dpr : 2 * dpr;
      ctx.strokeStyle = '#2c2822';
      ctx.beginPath();
      ctx.arc(cx, cy, R, -Math.PI / 2 + A0, -Math.PI / 2 + ang(T0));
      ctx.stroke();

      // Red zone: 0 → +3 VU — thick, matches physical reference meter
      ctx.lineWidth   = compact ? 3 * dpr : 5 * dpr;
      ctx.strokeStyle = '#b23a2c';
      ctx.beginPath();
      ctx.arc(cx, cy, R, -Math.PI / 2 + ang(T0), -Math.PI / 2 + A1);
      ctx.stroke();

      // ── Ticks ──────────────────────────────────────────────────────────────
      const MAJOR = new Set([-20, -10, -5, 0, 3]);
      VU_SCALE.forEach(m => {
        const a   = -Math.PI / 2 + ang(m.t);
        const red = m.v >= 0;
        const len = MAJOR.has(m.v)
          ? (compact ? 9 : 14) * dpr
          : (compact ? 5 : 8)  * dpr;
        ctx.strokeStyle = red ? '#b23a2c' : '#2c2822';
        ctx.lineWidth   = (red ? 2 : 1.2) * dpr;
        ctx.beginPath();
        ctx.moveTo(cx + Math.cos(a) * R,       cy + Math.sin(a) * R);
        ctx.lineTo(cx + Math.cos(a) * (R - len), cy + Math.sin(a) * (R - len));
        ctx.stroke();
      });

      if (!compact) {
        // ── Outer scale numbers (outside arc) ─────────────────────────────────
        // Convention: −20 with minus; others as absolute value; positive with +.
        const OUTER = [
          { v: -20, txt: '-20' }, { v: -10, txt: '10' }, { v: -7, txt: '7'  },
          { v:  -5, txt: '5'  }, { v:  -3, txt: '3'  }, { v:  0, txt: '0'  },
          { v:  +3, txt: '+3' },
        ];
        const fs = Math.round(R * 0.112);
        ctx.font         = `700 ${fs}px Inter, sans-serif`;
        ctx.textAlign    = 'center';
        ctx.textBaseline = 'middle';
        OUTER.forEach(m => {
          const a   = -Math.PI / 2 + ang(vuToT(m.v));
          const rr  = R + Math.round(R * 0.16);
          ctx.fillStyle = m.v >= 0 ? '#b23a2c' : '#2c2822';
          ctx.fillText(m.txt, cx + Math.cos(a) * rr, cy + Math.sin(a) * rr);
        });

        // ── Inner scale numbers (inside arc, dimmer, smaller) ──────────────────
        // Second row visible on the reference hardware meter.
        const INNER = [
          { v: -20, txt: '-20' }, { v: -10, txt: '-10' },
          { v:  -5, txt: '-5'  }, { v:   0, txt: '0'   },
        ];
        const fsI = Math.round(R * 0.088);
        ctx.font      = `400 ${fsI}px Inter, sans-serif`;
        ctx.fillStyle = '#6b6354';
        INNER.forEach(m => {
          const a  = -Math.PI / 2 + ang(vuToT(m.v));
          const rr = R - Math.round(R * 0.18);
          ctx.fillText(m.txt, cx + Math.cos(a) * rr, cy + Math.sin(a) * rr);
        });

        // ── Meter type label centered on face ──────────────────────────────────
        // Sits below the arc in the open space before the pivot — exactly where
        // "VU" is printed on a real hardware dial.
        const labelY = cy - Math.round(R * 0.40);
        ctx.font      = `700 ${Math.round(R * 0.12)}px Inter, sans-serif`;
        ctx.fillStyle = '#6b6354';
        ctx.fillText(labelRef.current, cx, labelY);
      }

      // ── Peak-hold needle (orange) ───────────────────────────────────────────
      if (holdVal !== null) {
        const ha = -Math.PI / 2 + ang(vuToT(holdVal));
        ctx.strokeStyle = '#e08a1e';
        ctx.lineWidth   = (compact ? 1.5 : 2) * dpr;
        ctx.beginPath();
        ctx.moveTo(cx + Math.cos(ha) * R * 0.52, cy + Math.sin(ha) * R * 0.52);
        ctx.lineTo(cx + Math.cos(ha) * (R - 3),  cy + Math.sin(ha) * (R - 3));
        ctx.stroke();
      }

      // ── Main needle ────────────────────────────────────────────────────────
      const na = -Math.PI / 2 + ang(vuToT(needleVal));
      ctx.strokeStyle = '#1b1814';
      ctx.lineWidth   = (compact ? 1.5 : 2.5) * dpr;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + Math.cos(na) * R * 0.97, cy + Math.sin(na) * R * 0.97);
      ctx.stroke();

      // ── Pivot hub ──────────────────────────────────────────────────────────
      const hub = (compact ? 5 : 9) * dpr;
      ctx.fillStyle = '#2c2822';
      ctx.beginPath(); ctx.arc(cx, cy, hub, 0, Math.PI * 2); ctx.fill();
      ctx.fillStyle = '#d4c8aa';
      ctx.beginPath(); ctx.arc(cx, cy, hub * 0.44, 0, Math.PI * 2); ctx.fill();
    }

    let lastTime = null;
    let simOffset = 0, simPhase = Math.random() * Math.PI * 2;

    function frame(ts) {
      if (!lastTime) lastTime = ts;
      const dt = Math.min((ts - lastTime) / 1000, 0.05);
      lastTime = ts;
      const s = stateRef.current;

      if (simulate) {
        simPhase += dt * (3 + Math.random());
        simOffset = Math.sin(simPhase) * 4 + Math.sin(simPhase * 2.7) * 2;
      }

      const target = targetRef.current + (simulate ? simOffset : 0);

      // Spring-physics VU ballistics (300 ms integration matches ANSI spec)
      const spring = 22, damp = 6;
      const acc = (target - s.needle) * spring - s.vel * damp;
      s.vel    += acc * dt;
      s.needle += s.vel * dt;

      if (simulate && s.needle > (s.peak ?? -99)) {
        s.peak = s.needle;
        s.peakTimer = 1.4;
      }
      if (s.peakTimer > 0) {
        s.peakTimer -= dt;
        if (s.peakTimer <= 0) s.peak = null;
      }

      draw(s.needle, peakHold ?? s.peak);
      animRef.current = requestAnimationFrame(frame);
    }

    animRef.current = requestAnimationFrame(frame);
    return () => { if (animRef.current) cancelAnimationFrame(animRef.current); };
  }, [compact, simulate]);

  const iw = width  - (compact ? 6 : 10);
  const ih = height - (compact ? 6 : 10);

  return (
    <div style={{
      width, height,
      borderRadius: compact ? 4 : 7,
      background: '#0d0d0f',
      padding: compact ? 3 : 5,
      boxSizing: 'border-box',
      flexShrink: 0,
    }}>
      <div style={{
        width: '100%', height: '100%',
        borderRadius: compact ? 3 : 5,
        background: 'linear-gradient(#f3ecd9, #d8cdb0)',
        position: 'relative',
      }}>
        <canvas
          ref={canvasRef}
          width={iw * 2} height={ih * 2}
          style={{ position: 'absolute', inset: 0, width: '100%', height: '100%', display: 'block' }}
        />
      </div>
    </div>
  );
}

Object.assign(window, { VuMeter });
