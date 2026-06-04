// ChannelStrip.jsx — Single-instance plugin window (360×520)
// Props: onOpenChannelView
// Exports: ChannelStrip

const { useState, useCallback } = React;

const METER_TYPES  = ['VU','RMS','RMS+3','K-12','K-14','K-20'];
const MONITOR_MODES = ['Stereo','Left','Right','Mono','Side'];
const BUCKET_COLORS = ['#8a94a3','#5b8cff','#36c1a6','#e2b341','#e5684d','#a56bff','#4db6e5','#8bc34a','#ff8a65'];

function Pill({ label, active, onClick, activeColor }) {
  return (
    <button onClick={onClick} style={{
      font:'700 10px/1 Inter, sans-serif', letterSpacing:'.035em', textTransform:'uppercase',
      padding:'5px 8px', borderRadius:3, cursor:'pointer', border:'1px solid',
      borderColor: active ? 'transparent' : '#2c333d',
      background: active ? (activeColor || '#36c1a6') : '#1d222a',
      color: active ? (activeColor ? '#fff' : '#11140f') : '#8a94a3',
      transition:'background 80ms, color 80ms',
    }}>{label}</button>
  );
}

function ToggleBtn({ label, active, onClick, onColor, textColor }) {
  return (
    <button onClick={onClick} style={{
      flex:1, font:'700 11px/1 Inter, sans-serif', letterSpacing:'.04em', textTransform:'uppercase',
      padding:'8px 0', borderRadius:3, cursor:'pointer', border:'1px solid',
      borderColor: active ? 'transparent' : '#2c333d',
      background: active ? (onColor||'#36c1a6') : '#1d222a',
      color: active ? (textColor||'#11140f') : '#e7ecf2',
      transition:'background 80ms',
    }}>{label}</button>
  );
}

function ChannelStrip({ onOpenChannelView }) {
  const [bypass,  setBypass]  = useState(false);
  const [mute,    setMute]    = useState(false);
  const [mono,    setMono]    = useState(false);
  const [polar,   setPolar]   = useState(false);
  const [gain,    setGain]    = useState(0);
  const [mType,   setMType]   = useState('VU');
  const [monitor, setMonitor] = useState('Stereo');
  const [group,   setGroup]   = useState(0);
  const [name,    setName]    = useState('Channel 1');

  // Simulate a healthy VU level around -9 dB
  const vuTarget = bypass ? -40 : mute ? -40 : Math.max(-20, Math.min(3, -9 + gain * 0.5));

  const col = BUCKET_COLORS[group];

  return (
    <div style={{
      width:360, height:520, background:'#14171c', display:'flex', flexDirection:'column',
      fontFamily:'Inter, sans-serif', color:'#e7ecf2', boxSizing:'border-box',
      borderRadius:6, overflow:'hidden',
      boxShadow:'0 12px 40px rgba(0,0,0,.6), 0 2px 8px rgba(0,0,0,.4)',
      opacity: bypass ? 0.82 : 1, transition:'opacity 120ms',
    }}>

      {/* ── Header ── */}
      <div style={{
        height:34, background:'#0f1217', display:'flex', alignItems:'center',
        padding:'0 12px', gap:8, flexShrink:0, borderBottom:'1px solid #2c333d',
      }}>
        <div style={{ width:8, height:8, borderRadius:'50%', background:col, flexShrink:0 }} />
        <div style={{ font:'700 16px/1 Inter, sans-serif', color:'#36c1a6', letterSpacing:'.06em' }}>STRATA</div>
        <div style={{ flex:1 }} />
        <button onClick={onOpenChannelView} style={{
          font:'700 9px/1 Inter, sans-serif', letterSpacing:'.04em', textTransform:'uppercase',
          padding:'4px 8px', borderRadius:3, cursor:'pointer',
          border:'1px solid #2c333d', background:'#1d222a', color:'#8a94a3',
        }}>CH VIEW</button>
      </div>

      {/* ── Body ── */}
      <div style={{ flex:1, display:'flex', flexDirection:'column', padding:'10px 12px 10px', gap:8, overflow:'hidden' }}>

        {/* Name field */}
        <input
          value={name} onChange={e=>setName(e.target.value)}
          style={{
            font:'400 13px/1 Inter, sans-serif', color:'#e7ecf2', background:'#1d222a',
            border:'1px solid #2c333d', borderRadius:3, padding:'6px 10px',
            outline:'none', width:'100%', boxSizing:'border-box',
          }}
        />

        {/* VU meter — full width */}
        <VuMeter value={vuTarget} width={336} height={178} simulate={!bypass && !mute} />

        {/* Meter type selector */}
        <div style={{ display:'flex', gap:4 }}>
          {METER_TYPES.map(t => (
            <Pill key={t} label={t} active={mType===t} onClick={()=>setMType(t)} />
          ))}
        </div>

        {/* Monitor selector */}
        <div style={{ display:'flex', gap:4 }}>
          {MONITOR_MODES.map(m => (
            <Pill key={m} label={m} active={monitor===m} onClick={()=>setMonitor(m)} />
          ))}
        </div>

        {/* INPUT GAIN knob — centered */}
        <div style={{ display:'flex', justifyContent:'center', padding:'4px 0' }}>
          <Knob value={gain} min={-40} max={12} onChange={setGain} label="INPUT GAIN" size={78} />
        </div>

        {/* Toggle buttons */}
        <div style={{ display:'flex', gap:6 }}>
          <ToggleBtn label="BYPASS" active={bypass} onClick={()=>setBypass(v=>!v)} onColor="#e2b341" textColor="#1a160a" />
          <ToggleBtn label="MUTE"   active={mute}   onClick={()=>setMute(v=>!v)}   onColor="#e5484d" textColor="#fff" />
          <ToggleBtn label="MONO"   active={mono}   onClick={()=>setMono(v=>!v)} />
          <ToggleBtn label="Ø"      active={polar}  onClick={()=>setPolar(v=>!v)} />
        </div>

        {/* Group selector */}
        <div style={{ display:'flex', alignItems:'center', gap:6, paddingTop:2 }}>
          <div style={{ font:'700 9px/1 Inter, sans-serif', color:'#8a94a3', letterSpacing:'.04em', textTransform:'uppercase', marginRight:2 }}>GROUP</div>
          {BUCKET_COLORS.map((c,i) => (
            <div key={i} onClick={()=>setGroup(i)} style={{
              width: group===i?12:9, height: group===i?12:9,
              borderRadius:'50%', background:c, cursor:'pointer',
              outline: group===i ? `2px solid ${c}` : 'none',
              outlineOffset:2,
              transition:'all 80ms',
              opacity: group===i ? 1 : 0.55,
            }} />
          ))}
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { ChannelStrip, BUCKET_COLORS });
