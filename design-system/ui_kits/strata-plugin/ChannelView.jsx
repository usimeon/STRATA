// ChannelView.jsx — Multi-instance control surface
// Exports: ChannelView, Fader

const { useState, useCallback } = React;

// ── Fader ──────────────────────────────────────────────────────────────────
// Bold wide-rib cap (like reference image): thick horizontal slab bands,
// dark shadow on top of each rib / light highlight on bottom = raised 3D look.
function Fader({ db=0, min=-60, max=12, onChange, health='green', trackH=120 }) {
  const CAP_H = 52; // tall cap — wide ribs need height
  const RIB_H = 10; // each rib band
  const t      = (db - min) / (max - min);
  const capTop = Math.round((1 - t) * (trackH - CAP_H));
  const healthColors = { red:'#e5484d', amber:'#e2b341', green:'#3fb86b', blue:'#4178d6', grey:'#5b5f6b' };

  const scaleMarks = [];
  for (let v = max; v >= min; v -= 6) scaleMarks.push(v);
  const tOf = v => (v - min) / (max - min);

  const onMouseDown = e => {
    e.preventDefault();
    const rect = e.currentTarget.getBoundingClientRect();
    const move = ev => {
      const y = ev.clientY - rect.top;
      const nt = 1 - Math.max(0, Math.min(1, y / trackH));
      onChange && onChange(Math.round((min + nt*(max-min)) * 10) / 10);
    };
    const up = () => { window.removeEventListener('mousemove', move); window.removeEventListener('mouseup', up); };
    window.addEventListener('mousemove', move);
    window.addEventListener('mouseup', up);
  };

  // Wide-rib cap background:
  // Each "rib" = shadow strip (dark top) + body + highlight strip (light bottom)
  // Gives a physically raised slab look.
  const ribBg = `
    repeating-linear-gradient(
      to bottom,
      rgba(0,0,0,.38)      0px,
      rgba(0,0,0,.38)      2px,
      transparent          2px,
      transparent          ${RIB_H - 2}px,
      rgba(255,255,255,.28) ${RIB_H - 2}px,
      rgba(255,255,255,.28) ${RIB_H}px
    ),
    linear-gradient(to bottom, #c8c8cc, #a8a8ac 48%, #989899 52%, #b0b0b4)
  `;

  return (
    <div style={{ display:'flex', gap:5, userSelect:'none', alignItems:'flex-start' }}>

      {/* ── Track + cap area ── */}
      <div style={{ position:'relative', width:46, height:trackH, cursor:'ns-resize', flexShrink:0 }} onMouseDown={onMouseDown}>

        {/* Health wash */}
        <div style={{ position:'absolute', left:'50%', transform:'translateX(-50%)', bottom:0, width:9, height:`${Math.round(t*100)}%`, borderRadius:2, background:healthColors[health]||'#5b5f6b', opacity:.38 }} />

        {/* Recessed slot */}
        <div style={{
          position:'absolute', left:'50%', transform:'translateX(-50%)',
          top: CAP_H/2, bottom: CAP_H/2,
          width:5, borderRadius:2,
          background:'#080a0c',
          boxShadow:'inset 0 0 5px rgba(0,0,0,.9), inset 1px 0 0 rgba(255,255,255,.04)',
        }} />

        {/* Wide-rib cap */}
        <div style={{
          position:'absolute', left:0, right:0, height:CAP_H, top:capTop,
          borderRadius:3,
          background: ribBg,
          boxShadow:'0 4px 8px rgba(0,0,0,.6), 0 1px 0 rgba(255,255,255,.3) inset',
          overflow:'hidden',
        }}>
          {/* Center index line */}
          <div style={{
            position:'absolute', left:'50%', top:'50%',
            width:30, height:2,
            transform:'translate(-50%,-50%)',
            background:'rgba(0,0,0,.45)',
            borderRadius:1,
          }} />
        </div>
      </div>

      {/* ── dB scale ── */}
      <div style={{ position:'relative', height:trackH, width:18, flexShrink:0 }}>
        {scaleMarks.map(v => {
          const yPct = (1 - tOf(v)) * 100;
          const isZero = v === 0;
          return (
            <div key={v} style={{
              position:'absolute', top:`${yPct}%`, transform:'translateY(-50%)',
              font:`${isZero?700:400} 8px/1 Inter, sans-serif`,
              color: isZero ? '#8a94a3' : '#4a5060',
              fontVariantNumeric:'tabular-nums', whiteSpace:'nowrap',
            }}>
              {v === 0 ? '0' : Math.abs(v)}
            </div>
          );
        })}
      </div>

    </div>
  );
}

// ── DotHandle ─────────────────────────────────────────────────────────────
function DotHandle() {
  return (
    <div style={{ display:'grid', gridTemplateColumns:'repeat(2, 3px)', gap:'3px', padding:'2px', cursor:'grab', alignSelf:'flex-start' }}>
      {[...Array(6)].map((_,i) => <div key={i} style={{ width:3, height:3, borderRadius:'50%', background:'#8a94a3' }} />)}
    </div>
  );
}

// ── SmBtn ──────────────────────────────────────────────────────────────────
function SmBtn({ label, active, onClick, onBg='#36c1a6', onTxt='#11140f' }) {
  return (
    <button onClick={onClick} style={{
      font:'700 9px/1 Inter, sans-serif', letterSpacing:'.04em', textTransform:'uppercase',
      padding:'4px 5px', borderRadius:2, cursor:'pointer', border:'1px solid',
      borderColor: active ? 'transparent' : '#2c333d',
      background: active ? onBg : '#14171c',
      color: active ? onTxt : '#e7ecf2',
    }}>{label}</button>
  );
}

// ── ChannelColumn ─────────────────────────────────────────────────────────
function ChannelColumn({ ch, isSelf, onChange }) {
  return (
    <div style={{
      width:84, background:'#1d222a', borderRadius:5,
      border: isSelf ? '1.5px solid #36c1a6' : '1px solid #2c333d',
      padding:'7px 6px', display:'flex', flexDirection:'column',
      alignItems:'center', gap:5,
      opacity: ch.bypass ? 0.62 : 1,
      transition:'opacity 120ms',
    }}>
      <DotHandle />
      <input
        value={ch.name} onChange={e => onChange({ name: e.target.value })}
        style={{
          font:'400 11px/1.2 Inter, sans-serif', color:'#e7ecf2',
          background:'#14171c', border:'1px solid #2c333d', borderRadius:3,
          padding:'3px 5px', width:'100%', boxSizing:'border-box', textAlign:'center',
        }}
      />
      {/* Mini VU */}
      <VuMeter
        value={ch.bypass||ch.mute ? -40 : ch.db + (ch.gain||0) * 0.5}
        width={70} height={48}
        compact={true}
        simulate={!ch.bypass && !ch.mute}
      />
      {/* Fader */}
      <Fader db={ch.gain||0} health={ch.health} onChange={db => onChange({ gain: db })} />
      {/* IN / LINK buttons */}
      <div style={{ display:'flex', gap:4 }}>
        <SmBtn label="IN"   active={ch.inOn}   onClick={()=>onChange({inOn:!ch.inOn})}   onBg="#e2b341" onTxt="#1a160a" />
        <SmBtn label={`L${ch.link}`} active={ch.linkOn} onClick={()=>onChange({linkOn:!ch.linkOn})} onBg="#5b8cff" onTxt="#fff" />
      </div>
    </div>
  );
}

// ── BucketTab ──────────────────────────────────────────────────────────────
function BucketTab({ label, color, active, onClick, count }) {
  return (
    <div onClick={onClick} style={{
      display:'flex', alignItems:'center', gap:6, padding:'6px 12px 5px',
      borderRadius:'3px 3px 0 0', cursor:'pointer',
      background: active ? `color-mix(in srgb, ${color} 22%, #1d222a)` : '#14171c',
      border:'1px solid #2c333d', borderBottom: active ? '1px solid transparent' : undefined,
      position:'relative', userSelect:'none',
    }}>
      {active && (
        <div style={{ position:'absolute', top:0, left:0, right:0, height:3, borderRadius:'3px 3px 0 0', background:color }} />
      )}
      <div style={{ width:7, height:7, borderRadius:'50%', background:color, flexShrink:0 }} />
      <div style={{ font:'400 11px/1 Inter, sans-serif', color: active ? '#e7ecf2' : '#8a94a3' }}>{label}</div>
      {count > 0 && <div style={{ font:'700 10px/1 Inter, sans-serif', color: active ? '#36c1a6' : '#5b5f6b' }}>{count}</div>}
    </div>
  );
}

// ── ChannelView ────────────────────────────────────────────────────────────
const BUCKETS = [
  { id:0, label:'All',   color:'#8a94a3' },
  { id:1, label:'Drums', color:'#5b8cff' },
  { id:2, label:'Keys',  color:'#36c1a6' },
  { id:4, label:'FX',    color:'#e5684d' },
  { id:5, label:'Vox',   color:'#a56bff' },
];

const INIT_CHANNELS = [
  { id:1, name:'Kick In',  group:1, db:-9,  gain:0,  health:'green', inOn:true,  linkOn:true,  link:1, bypass:false, mute:false },
  { id:2, name:'Snare',    group:1, db:-6,  gain:2,  health:'amber', inOn:true,  linkOn:false, link:1, bypass:false, mute:false },
  { id:3, name:'OH L',     group:1, db:-14, gain:0,  health:'green', inOn:true,  linkOn:true,  link:2, bypass:false, mute:false },
  { id:4, name:'OH R',     group:1, db:-14, gain:0,  health:'green', inOn:true,  linkOn:true,  link:2, bypass:false, mute:false },
  { id:5, name:'Hi-Hat',   group:1, db:-18, gain:-3, health:'blue',  inOn:false, linkOn:false, link:1, bypass:false, mute:false },
  { id:6, name:'Piano L',  group:2, db:-10, gain:0,  health:'green', inOn:true,  linkOn:true,  link:3, bypass:false, mute:false },
  { id:7, name:'Piano R',  group:2, db:-10, gain:0,  health:'green', inOn:true,  linkOn:true,  link:3, bypass:false, mute:false },
  { id:8, name:'Vox Main', group:5, db:-7,  gain:1,  health:'amber', inOn:true,  linkOn:false, link:1, bypass:false, mute:false },
  { id:9, name:'BGV',      group:5, db:-12, gain:-2, health:'green', inOn:true,  linkOn:false, link:1, bypass:false, mute:false },
  { id:10,name:'Rev',      group:4, db:-20, gain:0,  health:'grey',  inOn:false, linkOn:false, link:1, bypass:false, mute:false },
];

function ChannelView({ onClose, selfId = 1 }) {
  const [activeBucket, setActiveBucket] = useState(1);
  const [channels, setChannels] = useState(INIT_CHANNELS);

  const updateCh = useCallback((id, patch) => {
    setChannels(chs => chs.map(c => c.id === id ? { ...c, ...patch } : c));
  }, []);

  const visible = activeBucket === 0
    ? channels
    : channels.filter(c => c.group === activeBucket);

  return (
    <div style={{
      background:'#14171c', borderRadius:6, overflow:'hidden',
      boxShadow:'0 16px 48px rgba(0,0,0,.7)',
      fontFamily:'Inter, sans-serif', color:'#e7ecf2',
      display:'inline-flex', flexDirection:'column',
      minWidth: 440,
    }}>
      {/* Header */}
      <div style={{
        height:34, background:'#0f1217', display:'flex', alignItems:'center',
        padding:'0 14px', gap:10, borderBottom:'1px solid #2c333d',
      }}>
        <div style={{ font:'700 16px/1 Inter, sans-serif', color:'#36c1a6', letterSpacing:'.06em' }}>STRATA</div>
        <div style={{ font:'700 9px/1 Inter, sans-serif', color:'#36c1a6', letterSpacing:'.08em', border:'1px solid #36c1a6', borderRadius:2, padding:'2px 5px' }}>CH VIEW</div>
        <div style={{ flex:1 }} />
        <button onClick={onClose} style={{
          font:'700 9px/1 Inter, sans-serif', letterSpacing:'.04em',
          padding:'4px 8px', borderRadius:3, cursor:'pointer',
          border:'1px solid #2c333d', background:'#1d222a', color:'#8a94a3',
        }}>STRIP VIEW</button>
      </div>

      {/* Tabs */}
      <div style={{ display:'flex', gap:2, padding:'8px 12px 0', background:'#14171c' }}>
        {BUCKETS.map(b => (
          <BucketTab
            key={b.id} label={b.label} color={b.color}
            active={activeBucket===b.id}
            count={b.id===0?channels.length:channels.filter(c=>c.group===b.id).length}
            onClick={()=>setActiveBucket(b.id)}
          />
        ))}
      </div>

      {/* Columns */}
      <div style={{
        background:'#1d222a', borderTop:'1px solid #2c333d',
        padding:'12px 14px', display:'flex', gap:10, flexWrap:'wrap',
      }}>
        {visible.length === 0 ? (
          <div style={{ padding:'20px 12px', font:'400 12px/1.5 Inter, sans-serif', color:'#5b5f6b', minWidth:240 }}>
            No channels in this bucket yet.<br />Use ASSIGN to add STRATA instances.
          </div>
        ) : visible.map(ch => (
          <ChannelColumn key={ch.id} ch={ch} isSelf={ch.id===selfId} onChange={patch=>updateCh(ch.id,patch)} />
        ))}
      </div>
    </div>
  );
}

Object.assign(window, { ChannelView, Fader });
