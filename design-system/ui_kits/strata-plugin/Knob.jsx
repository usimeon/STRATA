// Knob.jsx — Rotary knob with teal value arc, draggable
// Export: Knob

const { useRef, useEffect, useCallback } = React;

function Knob({ value = 0, min = -40, max = 12, onChange, label = 'INPUT GAIN', size = 76, self: isSelf = false }) {
  const canvasRef = useRef(null);

  const draw = useCallback(() => {
    const canvas = canvasRef.current; if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;
    const cx = W/2, cy = H/2;
    const R = W * 0.38;
    const START = -Math.PI * 0.72, END = Math.PI * 0.72;
    const t = (value - min) / (max - min);
    const valAngle = START + t * (END - START);

    ctx.clearRect(0, 0, W, H);

    // Self outline
    if (isSelf) {
      ctx.strokeStyle = '#36c1a6'; ctx.lineWidth = 3;
      ctx.beginPath(); ctx.arc(cx, cy, R + 5, 0, Math.PI*2); ctx.stroke();
    }

    // Body
    const bodyGrad = ctx.createRadialGradient(cx-R*0.2, cy-R*0.2, 2, cx, cy, R);
    bodyGrad.addColorStop(0, '#272e38');
    bodyGrad.addColorStop(1, '#1a1f27');
    ctx.fillStyle = bodyGrad;
    ctx.beginPath(); ctx.arc(cx, cy, R, 0, Math.PI*2); ctx.fill();

    // Body edge
    ctx.strokeStyle='rgba(255,255,255,.06)'; ctx.lineWidth=1;
    ctx.beginPath(); ctx.arc(cx, cy, R, 0, Math.PI*2); ctx.stroke();

    // Track
    ctx.strokeStyle='#2c333d'; ctx.lineWidth=4.5;
    ctx.lineCap='round';
    ctx.beginPath(); ctx.arc(cx, cy, R-8, START, END); ctx.stroke();

    // Value arc (teal)
    ctx.strokeStyle='#36c1a6'; ctx.lineWidth=4.5;
    ctx.beginPath(); ctx.arc(cx, cy, R-8, START, valAngle); ctx.stroke();
    ctx.lineCap='butt';

    // Pointer line
    const px = cx + Math.cos(valAngle) * (R - 13);
    const py = cy + Math.sin(valAngle) * (R - 13);
    ctx.strokeStyle='#e7ecf2'; ctx.lineWidth=2;
    ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(px, py); ctx.stroke();

    // Hub dot
    ctx.fillStyle='#2c333d';
    ctx.beginPath(); ctx.arc(cx, cy, 6, 0, Math.PI*2); ctx.fill();
  }, [value, min, max, isSelf]);

  useEffect(() => { draw(); }, [draw]);

  const onMouseDown = useCallback(e => {
    e.preventDefault();
    const startY = e.clientY, startVal = value;
    const range = max - min;
    const move = ev => {
      const dy = startY - ev.clientY;
      const nv = Math.max(min, Math.min(max, startVal + dy * (range / 220)));
      onChange && onChange(Math.round(nv * 10) / 10);
    };
    const up = () => { window.removeEventListener('mousemove', move); window.removeEventListener('mouseup', up); };
    window.addEventListener('mousemove', move);
    window.addEventListener('mouseup', up);
  }, [value, min, max, onChange]);

  const onDblClick = useCallback(() => { onChange && onChange(0); }, [onChange]);

  const disp = value === 0 ? '0 dB' : value > 0 ? `+${value} dB` : `${value} dB`;

  return (
    <div style={{ display:'flex', flexDirection:'column', alignItems:'center', gap:4, userSelect:'none' }}>
      <canvas
        ref={canvasRef}
        width={size*2} height={size*2}
        style={{ width:size, height:size, cursor:'ns-resize', display:'block' }}
        onMouseDown={onMouseDown}
        onDoubleClick={onDblClick}
        title="Drag to adjust · double-click to reset"
      />
      <div style={{ font:`700 9px/1 Inter, sans-serif`, letterSpacing:'.04em', color:'#8a94a3', textTransform:'uppercase' }}>{label}</div>
      <div style={{ font:`400 9px/1 Inter, sans-serif`, color:'#e7ecf2', fontVariantNumeric:'tabular-nums' }}>{disp}</div>
    </div>
  );
}

Object.assign(window, { Knob });
