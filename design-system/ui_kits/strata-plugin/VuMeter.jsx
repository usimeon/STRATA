// VuMeter.jsx — Analog VU meter with spring-physics needle + optional live simulation
// Export: VuMeter
// Props: value (dB, -20..+3), peakHold (dB|null), width, height, compact, simulate

const { useRef, useEffect } = React;

function VuMeter({ value = -12, peakHold = null, width = 300, height = 180, compact = false, simulate = false }) {
  const canvasRef = useRef(null);
  const stateRef  = useRef({ needle: value, vel: 0, peak: peakHold, peakTimer: 0 });
  const animRef   = useRef(null);
  const targetRef = useRef(value);

  // Update target when prop changes
  useEffect(() => { targetRef.current = value; }, [value]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;
    const cx = W / 2, cy = Math.round(H * 0.942);
    const R = compact ? Math.round(W * 0.36) : Math.round(W * 0.385);
    const A0 = -0.88, A1 = 0.88;
    const ang = t => A0 + t * (A1 - A0);

    // Non-linear VU arc positions
    const vuScale = [
      {v:-20,t:0},{v:-10,t:0.218},{v:-7,t:0.324},{v:-5,t:0.41},
      {v:-3,t:0.508},{v:-2,t:0.57},{v:-1,t:0.636},{v:0,t:0.71},
      {v:1,t:0.793},{v:2,t:0.87},{v:3,t:1},
    ];
    const pctScale = [{p:20,t:0.131},{p:40,t:0.29},{p:60,t:0.437},{p:80,t:0.574},{p:100,t:0.71}];
    const T0 = 0.71;

    function vuToT(v) {
      const c = Math.max(-20, Math.min(3, v));
      for (let i = 0; i < vuScale.length - 1; i++) {
        const a = vuScale[i], b = vuScale[i+1];
        if (c >= a.v && c <= b.v) return a.t + (c - a.v) / (b.v - a.v) * (b.t - a.t);
      }
      return c <= -20 ? 0 : 1;
    }

    function draw(needleVal, holdVal) {
      ctx.clearRect(0, 0, W, H);

      // Main arc (black → red at 0 VU)
      ctx.lineWidth = compact ? 2 : 2.5;
      ctx.strokeStyle = '#3a3028';
      ctx.beginPath(); ctx.arc(cx,cy,R,-Math.PI/2+A0,-Math.PI/2+ang(T0)); ctx.stroke();
      ctx.lineWidth = compact ? 2.5 : 3;
      ctx.strokeStyle = '#b23a2c';
      ctx.beginPath(); ctx.arc(cx,cy,R,-Math.PI/2+ang(T0),-Math.PI/2+A1); ctx.stroke();

      // Major ticks
      const majors = new Set([-20,-10,-5,0,3]);
      vuScale.forEach(m => {
        const a = -Math.PI/2 + ang(m.t), red = m.v >= 0;
        const len = majors.has(m.v) ? (compact?9:14) : (compact?5:9);
        ctx.strokeStyle = red ? '#b23a2c' : '#3a3028';
        ctx.lineWidth = red ? 2 : 1.5;
        ctx.beginPath();
        ctx.moveTo(cx+Math.cos(a)*R, cy+Math.sin(a)*R);
        ctx.lineTo(cx+Math.cos(a)*(R-len), cy+Math.sin(a)*(R-len));
        ctx.stroke();
      });

      // Minor ticks (full-size only)
      if (!compact) {
        for (let i=0;i<vuScale.length-1;i++) {
          const t1=vuScale[i].t, t2=vuScale[i+1].t;
          for (let j=1;j<3;j++) {
            const t=t1+j*(t2-t1)/3, a=-Math.PI/2+ang(t), red=t>=T0;
            ctx.strokeStyle = red ? 'rgba(178,58,44,.5)' : 'rgba(58,48,40,.5)';
            ctx.lineWidth=1;
            ctx.beginPath();
            ctx.moveTo(cx+Math.cos(a)*R, cy+Math.sin(a)*R);
            ctx.lineTo(cx+Math.cos(a)*(R-5), cy+Math.sin(a)*(R-5));
            ctx.stroke();
          }
        }

        // VU numbers (outside arc, no minus sign).
        // -20 is at the extreme left end — skip its number label (leave the tick);
        // a tick alone is standard on real VU meters and avoids collision with the "VU" text.
        ctx.textAlign='center'; ctx.textBaseline='middle';
        const fs = Math.round(R*0.118);
        ctx.font = `700 ${fs}px Inter, sans-serif`;
        vuScale.forEach(m => {
          if (m.v === -20) return; // tick only, no label at far-left end
          const a=-Math.PI/2+ang(m.t), rr=R+Math.round(R*0.155);
          ctx.fillStyle = m.v>=0 ? '#b23a2c' : '#2c2822';
          ctx.fillText(String(Math.abs(m.v)), cx+Math.cos(a)*rr, cy+Math.sin(a)*rr);
        });

        // % numbers (inside arc)
        ctx.font = `400 ${Math.round(R*0.09)}px Inter, sans-serif`;
        ctx.fillStyle = '#7a7060';
        pctScale.forEach(m => {
          const a=-Math.PI/2+ang(m.t), rr=R-Math.round(R*0.175);
          ctx.fillText(String(m.p), cx+Math.cos(a)*rr, cy+Math.sin(a)*rr);
        });

        // "VU" label — sits at the far-left end where the -20 tick is,
        // shifted slightly inward so it doesn't clip the bezel.
        ctx.textAlign='center';
        ctx.font=`700 ${Math.round(R*0.12)}px Inter, sans-serif`;
        ctx.fillStyle='#7a7060';
        const vuA = -Math.PI/2 + ang(0); // angle of the -20 end
        ctx.fillText('VU',
          cx + Math.cos(vuA) * (R + Math.round(R*0.155)),
          cy + Math.sin(vuA) * (R + Math.round(R*0.155)) - Math.round(R*0.14)
        );
      }

      // Peak-hold needle (orange)
      if (holdVal !== null) {
        const ha = -Math.PI/2 + ang(vuToT(holdVal));
        ctx.strokeStyle='#e08a1e'; ctx.lineWidth=compact?1.5:2;
        ctx.beginPath();
        ctx.moveTo(cx+Math.cos(ha)*R*0.52, cy+Math.sin(ha)*R*0.52);
        ctx.lineTo(cx+Math.cos(ha)*(R-3),  cy+Math.sin(ha)*(R-3));
        ctx.stroke();
      }

      // Main needle
      const na = -Math.PI/2 + ang(vuToT(needleVal));
      ctx.strokeStyle='#1b1814'; ctx.lineWidth=compact?1.5:2.5;
      ctx.beginPath();
      ctx.moveTo(cx,cy); ctx.lineTo(cx+Math.cos(na)*R*0.97, cy+Math.sin(na)*R*0.97);
      ctx.stroke();

      // Hub
      ctx.fillStyle='#2c2822'; ctx.beginPath(); ctx.arc(cx,cy,compact?5:9,0,Math.PI*2); ctx.fill();
      ctx.fillStyle='#d4c8aa'; ctx.beginPath(); ctx.arc(cx,cy,compact?2.5:4,0,Math.PI*2); ctx.fill();
    }

    let lastTime = null;
    let simOffset = 0, simPhase = Math.random() * Math.PI * 2;

    function frame(ts) {
      if (!lastTime) lastTime = ts;
      const dt = Math.min((ts - lastTime) / 1000, 0.05);
      lastTime = ts;
      const s = stateRef.current;

      // Simulate signal fluctuation
      if (simulate) {
        simPhase += dt * (3 + Math.random());
        simOffset = Math.sin(simPhase) * 4 + Math.sin(simPhase * 2.7) * 2;
      }

      const target = targetRef.current + (simulate ? simOffset : 0);

      // VU ballistics: spring physics
      const spring = 22, damp = 6;
      const acc = (target - s.needle) * spring - s.vel * damp;
      s.vel += acc * dt;
      s.needle += s.vel * dt;

      // Peak hold
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
    <div style={{ width, height, borderRadius: compact?4:7, background:'#0d0d0f', padding: compact?3:5, boxSizing:'border-box', flexShrink:0 }}>
      <div style={{ width:'100%', height:'100%', borderRadius:compact?3:5, background:'linear-gradient(#f3ecd9,#d8cdb0)', position:'relative' }}>
        <canvas
          ref={canvasRef}
          width={iw * 2} height={ih * 2}
          style={{ position:'absolute', inset:0, width:'100%', height:'100%', display:'block' }}
        />
      </div>
    </div>
  );
}

Object.assign(window, { VuMeter });
