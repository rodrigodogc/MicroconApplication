#pragma once
static const char paginaParametros[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no" />
<title>Painel • Configuração & Controle</title>
<style>
  :root{
    --blue-900:#0a2540; --blue-800:#113a6b; --blue-700:#1a4d8f;
    --blue-600:#2a6fdb; --blue-500:#3b82f6; --blue-400:#60a5fa; --blue-300:#93c5fd;
    --ink-900:#0b1220;
    --bg-grad: radial-gradient(1200px 900px at 20% -10%, rgba(59,130,246,.25), transparent 60%),
               radial-gradient(1200px 900px at 120% 10%, rgba(37,99,235,.30), transparent 60%),
               linear-gradient(180deg, #0b1220 0%, #0a2540 100%);
    --glass: rgba(255,255,255,.08);
    --success:#22c55e; --err:#ef4444;
  }
  *{box-sizing:border-box; -webkit-tap-highlight-color:transparent}
  body{
    margin:0; font:15px/1.45 ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Ubuntu;
    color:#e8eefc; background:var(--bg-grad); min-height:100vh; display:flex; flex-direction:column;
  }
  /* Topbar */
  .topbar{
    position:sticky; top:0; z-index:50;
    display:flex; align-items:center; justify-content:space-between; gap:10px;
    padding:12px 16px; backdrop-filter:saturate(140%) blur(10px);
    background:linear-gradient(180deg, rgba(10,37,64,.9), rgba(10,37,64,.55));
    border-bottom:1px solid rgba(255,255,255,.06);
  }
  .brand{display:flex; align-items:center; gap:10px; min-width:0}
  .logo{
    width:32px; height:32px; border-radius:10px;
    background:conic-gradient(from 220deg, var(--blue-300), var(--blue-600), var(--blue-400));
    display:grid; place-items:center; box-shadow:0 6px 22px rgba(59,130,246,.35) inset; flex:0 0 auto;
    cursor:pointer; user-select:none; -webkit-user-select:none;
  }
  .logo svg{width:18px; height:18px; opacity:.92}
  .brand h1{margin:0; font-size:16px; font-weight:600; letter-spacing:.3px; color:#dbeafe; white-space:nowrap; overflow:hidden; text-overflow:ellipsis}
  .rightbar{display:flex; align-items:center; gap:10px}
  .ghost{
    display:inline-flex; align-items:center; gap:8px; text-decoration:none;
    padding:10px 12px; border-radius:12px; font-weight:600; letter-spacing:.2px;
    border:1px solid rgba(255,255,255,.12); color:#dbeafe; background:rgba(59,130,246,.12);
    transition:.2s transform,.2s background,.2s border-color; cursor:pointer;
  }
  .ghost:hover{transform:translateY(-1px); background:rgba(59,130,246,.16); border-color:rgba(255,255,255,.18)}

  /* Indicador de conexão */
  .conn{
    display:inline-flex; align-items:center; gap:8px;
    padding:8px 10px; border-radius:999px; font-weight:700; color:#dceaff; font-size:13px;
    background:rgba(255,255,255,.08); border:1px solid rgba(255,255,255,.12);
  }
  .conn .dot{width:10px; height:10px; border-radius:50%; background:#94a3b8}
  .conn.online .dot{ background:var(--success); animation:pulse 1.6s infinite ease-in-out; }
  @keyframes pulse{0%,100%{transform:scale(1);box-shadow:0 0 0 0 rgba(34,197,94,.45)}50%{transform:scale(1.15);box-shadow:0 0 0 6px rgba(34,197,94,.05)}}

  /* Local (pill no topo) */
  .local{
    display:inline-flex; align-items:center; gap:8px;
    padding:8px 10px; border-radius:999px; font-weight:700; color:#dceaff; font-size:13px;
    background:rgba(255,255,255,.08); border:1px solid rgba(255,255,255,.12);
    max-width:46vw; white-space:nowrap; overflow:hidden; text-overflow:ellipsis;
  }
  .local .ico{width:14px; height:14px; opacity:.9}

  .wrap{flex:1; display:grid; place-items:center; padding:20px}
  .card{
    width:min(920px, 96vw);
    background:var(--glass); border:1px solid rgba(255,255,255,.08);
    border-radius:20px; padding:18px; box-shadow:0 24px 80px rgba(3,24,66,.45), inset 0 1px 0 rgba(255,255,255,.06);
  }
  .grid{display:grid; gap:18px; grid-template-columns: 1.2fr 1fr}
  @media (max-width:880px){
    .grid{grid-template-columns:1fr}
    .wrap{padding:14px} .card{padding:14px}
    .local{max-width:54vw}
  }
  .section{background:rgba(255,255,255,.06); border:1px solid rgba(255,255,255,.08); border-radius:16px; padding:14px}
  .title{display:flex; align-items:center; justify-content:space-between; margin:0 0 10px; font-size:15px; color:#cfe4ff; font-weight:700; letter-spacing:.3px}

  /* Knob */
  .knob-wrap{display:grid; place-items:center; padding:10px 0}
  .knob{
    --size: clamp(180px, 46vw, 260px);
    width:var(--size); height:var(--size); border-radius:50%;
    position:relative; display:grid; place-items:center;
    background:
      radial-gradient(closest-side, rgba(255,255,255,.08), rgba(255,255,255,.03) 60%, transparent 61%) padding-box,
      conic-gradient(#60a5fa var(--pct), rgba(255,255,255,.10) 0) border-box;
    border: 10px solid transparent;
    background-clip: padding-box, border-box; /* evita sumiço >70% */
    box-shadow: inset 0 0 0 2px rgba(255,255,255,.08), 0 30px 70px rgba(0,0,0,.35), 0 0 0 1px rgba(255,255,255,.06);
    cursor:grab; user-select:none; touch-action:none;
  }
  .knob:active{cursor:grabbing}
  /* FIX: escopar o “dot” do knob para não afetar a bolinha de conexão */
  .knob .dot{
    position:absolute; width:10px; height:10px; border-radius:50%; background:#fff;
    box-shadow:0 0 0 6px rgba(96,165,250,.18), 0 0 0 1px rgba(255,255,255,.8) inset;
    transform: translate(-50%, -50%) rotate(var(--angle,-135deg)) translate(calc(var(--size)/2.45));
    top:50%; left:50%;
  }
  .knob-center{
    position:absolute; width:64%; height:64%; border-radius:50%;
    background:radial-gradient(circle at 30% 30%, rgba(255,255,255,.12), rgba(255,255,255,.03));
    border:1px solid rgba(255,255,255,.08);
    display:grid; place-items:center; text-align:center; padding:8px;
  }
  .readout .big{
    font-size:34px; font-weight:800; color:#e8f1ff;
    font-variant-numeric: tabular-nums; font-feature-settings: "tnum" 1, "lnum" 1;
    line-height:1; letter-spacing:.5px; min-width:8ch; text-align:center;
  }
  .readout .of{opacity:.7; margin-left:4px}
  .pill{display:inline-flex; align-items:center; gap:8px; padding:7px 10px;
    background:rgba(59,130,246,.14); border:1px solid rgba(255,255,255,.12);
    border-radius:999px; color:#dceaff; font-weight:600; letter-spacing:.2px; font-size:13px}
  .actions{display:flex; align-items:center; justify-content:flex-start; gap:10px; margin-top:10px}

  /* Temperatura (central, duas leituras) */
  .tempBox{display:grid; place-items:center; text-align:center; padding:18px 6px}
  .temps{display:grid; gap:10px; min-width:220px}
  .trow{display:flex; align-items:center; justify-content:center; gap:10px}
  .ico{width:20px; height:20px; opacity:.95}
  .tlabel{color:#bad6ff; font-weight:700; letter-spacing:.3px}
  .tval{display:flex; align-items:baseline; gap:8px; color:#eaf2ff}
  .tval .n{font-size:34px; font-weight:900}
  .tval small{color:#b6c6e8; font-size:16px}

  /* Toast único, embaixo */
  .status{position:fixed; left:50%; transform:translateX(-50%); bottom:14px; z-index:60; width: min(92vw, 520px);
    display:flex; justify-content:center; pointer-events:none}
  .toast{
    background:rgba(10,37,64,.90); border:1px solid rgba(255,255,255,.10); color:#e6f0ff;
    border-radius:12px; padding:10px 12px; font-size:13px; letter-spacing:.2px;
    box-shadow:0 14px 40px rgba(0,0,0,.35); display:flex; align-items:center; gap:10px; max-width:100%; pointer-events:auto
  }
  .toast.ok{border-color:rgba(34,197,94,.45)} .toast.err{border-color:rgba(239,68,68,.45)}

  /* Modal de Config */
  .modal-backdrop{position:fixed; inset:0; background:rgba(3,8,20,.6); backdrop-filter:blur(4px);
    display:none; align-items:center; justify-content:center; z-index:80}
  .modal{width:min(560px, 92vw); background:rgba(255,255,255,.06); border:1px solid rgba(255,255,255,.10);
    border-radius:16px; padding:14px; box-shadow:0 30px 70px rgba(0,0,0,.5)}
  .modal h3{margin:0 0 10px; font-size:18px; color:#e6f0ff}
  .fgrid{display:grid; gap:10px; grid-template-columns:1fr 1fr}
  .fgrid .full{grid-column:1/-1}
  .field{display:flex; flex-direction:column; gap:6px}
  .field label{font-size:12px; color:#b7c6e6}
  .field input{padding:12px 12px; border-radius:10px; border:1px solid rgba(255,255,255,.12);
    background:rgba(255,255,255,.08); color:#eaf2ff; outline:none; font-size:15px}
  .modal-actions{display:flex; justify-content:flex-end; gap:10px; margin-top:10px}

  /* Easter egg popup */
  .egg-backdrop{position:fixed; inset:0; background:rgba(3,8,20,.6); backdrop-filter:blur(3px);
    display:none; align-items:center; justify-content:center; z-index:90}
  .egg-modal{
    background:rgba(255,255,255,.08); border:1px solid rgba(255,255,255,.12);
    border-radius:14px; padding:14px 16px; color:#e6f0ff;
    box-shadow:0 30px 70px rgba(0,0,0,.5); text-align:center; font-weight:800;
  }

  @media (max-width:520px){
    .readout .big{font-size:30px} .tval .n{font-size:30px} .ghost{padding:9px 10px; border-radius:10px}
  }
</style>
</head>
<body>
  <header class="topbar">
    <div class="brand">
      <div class="logo" aria-hidden="true" id="logoEgg">
        <svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 2v6M12 16v6M4 12h6M14 12h6"/><circle cx="12" cy="12" r="5"></circle>
        </svg>
      </div>
      <h1>Controle de Ventilador • DEE</h1>
    </div>
    <div class="rightbar">
      <div id="localPill" class="local" style="display:none">
        <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
          <path d="M21 10c0 7-9 12-9 12S3 17 3 10a9 9 0 1 1 18 0Z"></path><circle cx="12" cy="10" r="3"></circle>
        </svg>
        <span id="localText"></span>
      </div>
      <div id="connTag" class="conn"><span class="dot"></span><span id="connText">Offline</span></div>
      <button id="btnConfig" class="ghost" title="Abrir configurações">Configurar</button>
    </div>
  </header>

  <main class="wrap">
    <section class="card">
      <div class="grid">
        <!-- Potenciômetro -->
        <div class="section">
          <h2 class="title">Potenciômetro (Duty)</h2>
          <div class="knob-wrap">
            <div id="knob" class="knob" role="slider" aria-valuemin="0" aria-valuemax="255" aria-valuenow="0" tabindex="0">
              <div class="dot"></div>
              <div class="knob-center">
                <div class="readout">
                  <div class="big"><span id="pctVal">0</span><span class="of">/100%</span></div>
                </div>
              </div>
            </div>
          </div>
          <div class="actions">
            <div class="pill">Velocidade Atual: <strong id="pillDuty">0%</strong></div>
          </div>
        </div>

        <!-- Temperaturas -->
        <div class="section tempBox">
          <h2 class="title" style="width:100%; justify-content:center;">Temperatura Atual</h2>
          <div class="temps">
            <div class="trow">
              <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M14 14.76V6a2 2 0 1 0-4 0v8.76a4 4 0 1 0 4 0Z"/><path d="M12 9v5"/></svg>
              <span class="tlabel">Interna</span>
              <span class="tval"><span class="n" id="tempInt">--.-</span><small>°C</small></span>
            </div>
            <div class="trow">
              <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M14 14.76V6a2 2 0 1 0-4 0v8.76a4 4 0 1 0 4 0Z"/><path d="M12 9v5"/></svg>
              <span class="tlabel">Externa</span>
              <span class="tval"><span class="n" id="tempExt">--.-</span><small>°C</small></span>
            </div>
          </div>
        </div>
      </div>
    </section>
  </main>

  <!-- Toast único -->
  <div class="status" id="status"></div>

  <!-- Modal Configurações (POST /configurar) -->
  <div class="modal-backdrop" id="modalConfig">
    <div class="modal">
      <h3>Configurações de Rede e MQTT</h3>
      <form id="formConfig">
        <div class="fgrid">
          <div class="field"><label>Wi-Fi (SSID)</label><input id="f_wifi" required placeholder="Nome da rede" /></div>
          <div class="field"><label>Senha Wi-Fi</label><input id="f_senha" type="password" placeholder="********" /></div>
          <div class="field"><label>IP do Broker</label><input id="f_ip" required placeholder="192.168.0.10" /></div>
          <div class="field"><label>Usuário MQTT</label><input id="f_user" placeholder="usuario (opcional)" /></div>
          <div class="field"><label>Senha MQTT</label><input id="f_pass" type="password" placeholder="senha (opcional)" /></div>
          <div class="field full"><label>Local</label><input id="f_local" placeholder="Ex.: Linha 1 - Ventilador A" /></div>
        </div>
        <div class="modal-actions">
          <button type="button" class="ghost" id="btnCloseConfig">Cancelar</button>
          <button type="submit" class="ghost">Salvar & Enviar</button>
        </div>
      </form>
    </div>
  </div>

  <!-- Easter Egg Popup -->
  <div class="egg-backdrop" id="egg">
    <div class="egg-modal">Criado por Rodrigo Silva :)</div>
  </div>

<script>
(function(){
  const knob = document.getElementById('knob');
  const pctVal  = document.getElementById('pctVal');
  const pillDuty = document.getElementById('pillDuty');
  const statusBox = document.getElementById('status');

  const tempIntEl = document.getElementById('tempInt');
  const tempExtEl = document.getElementById('tempExt');

  // Topbar conn & local
  const connTag = document.getElementById('connTag');
  const connText = document.getElementById('connText');
  const localPill = document.getElementById('localPill');
  const localText = document.getElementById('localText');

  // Config modal
  const btnConfig = document.getElementById('btnConfig');
  const modalConfig = document.getElementById('modalConfig');
  const btnCloseConfig = document.getElementById('btnCloseConfig');
  const formConfig = document.getElementById('formConfig');
  const fWifi = document.getElementById('f_wifi');
  const fSenha= document.getElementById('f_senha');
  const fIp   = document.getElementById('f_ip');
  const fUser = document.getElementById('f_user');
  const fPass = document.getElementById('f_pass');
  const fLocal= document.getElementById('f_local');

  // Estado
  let value = 0; // 0..255
  let dragging = false;
  let angleStart = 0; // ângulo (top-based) no início do drag
  let valueStart = 0; // valor inicial no início do drag
  let lastOkTs = 0;
  let lastToastTs = 0;

  const clamp = (n,min,max)=>Math.min(Math.max(n,min),max);
  const pctOf = v => Math.round((v/255)*100);

  function angleFromValue(v){ return -135 + 270*(v/255); }            // -135..+135
  function valueFromAngle(a){ const t=(a+135)/270; return Math.round(clamp(t,0,1)*255); }
  function normAngle(a){ while(a>180)a-=360; while(a<-180)a+=360; return a; }
  function angleDiff(a,b){ return normAngle(a-b); }
  function pointerAngleTop(e){
    const r = knob.getBoundingClientRect();
    const cx = r.left + r.width/2, cy = r.top + r.height/2;
    const dx = e.clientX - cx,     dy = e.clientY - cy;
    return Math.atan2(dx, -dy) * 180/Math.PI; // 0° topo, horário positivo
  }

  function setVisual(v){
    value = clamp(v,0,255);
    const pct = pctOf(value);
    const ang = angleFromValue(value);
    knob.style.setProperty('--pct', ((value/255)*100).toFixed(2) + '%'); // FIX 70%+
    knob.style.setProperty('--angle', ang+'deg');
    knob.setAttribute('aria-valuenow', value);
    pctVal.textContent = String(pct);
    pillDuty.textContent = pct + '%';
  }

  // Toast único
  function toast(msg, kind='ok'){
    statusBox.innerHTML = '';
    const el = document.createElement('div');
    el.className = 'toast '+(kind==='ok'?'ok':'err');
    el.textContent = msg;
    statusBox.appendChild(el);
    setTimeout(()=>{ el.style.opacity='0'; el.style.transform='translateY(4px)'; }, 1800);
    setTimeout(()=>{ el.remove(); }, 2300);
  }

  function updateConnVisual(){
    const online = (Date.now() - lastOkTs) < 5000;
    connTag.classList.toggle('online', online);
    connText.textContent = online ? 'Conectado' : 'Offline';
  }
  setInterval(updateConnVisual, 1000);

  // GET /set_dutycicle imediato
  let lastAbort = null;
  async function sendDuty(v){
    try{
      if (lastAbort) lastAbort.abort();
      lastAbort = new AbortController();
      const r = await fetch(`/set_dutycicle?dutycicle=${v}`, {signal:lastAbort.signal});
      if(!r.ok) throw new Error('HTTP '+r.status);
      lastOkTs = Date.now(); updateConnVisual();
      if (Date.now() - lastToastTs > 650){
        toast('Velocidade atualizada para '+pctOf(v)+'%', 'ok');
        lastToastTs = Date.now();
      }
    }catch(e){
      if (e.name === 'AbortError') return;
      toast('Falha ao atualizar velocidade', 'err');
    }
  }

  // Drag relativo (grab)
  knob.addEventListener('pointerdown', (e)=>{
    dragging = true;
    knob.setPointerCapture(e.pointerId);
    angleStart = pointerAngleTop(e);
    valueStart = value;
  });
  knob.addEventListener('pointermove', (e)=>{
    if(!dragging) return;
    const angNow = pointerAngleTop(e);
    const delta  = angleDiff(angNow, angleStart);
    const newAng = clamp(angleFromValue(valueStart) + delta, -135, 135);
    const v = valueFromAngle(newAng);
    setVisual(v);
    sendDuty(v);
  });
  knob.addEventListener('pointerup', (e)=>{
    dragging = false;
    try{knob.releasePointerCapture(e.pointerId);}catch(_){}
  });

  // Teclado (passos)
  knob.addEventListener('keydown', (e)=>{
    let step = (e.shiftKey? 15 : 5);
    if (e.key === 'ArrowRight' || e.key === 'ArrowUp'){ setVisual(value+step); sendDuty(value); }
    if (e.key === 'ArrowLeft'  || e.key === 'ArrowDown'){ setVisual(value-step); sendDuty(value); }
  });

  // Inicial
  setVisual(0);

  // Temperaturas (via firmware REST que lê MQTT)
  async function refreshTemps(){
    try{
      const r = await fetch('/get_temperaturas');
      if(!r.ok) throw new Error();
      const j = await r.json();
      const ti = parseFloat(j.interna);
      const te = parseFloat(j.externa);
      if (!isNaN(ti)) tempIntEl.textContent = ti.toFixed(1);
      if (!isNaN(te)) tempExtEl.textContent = te.toFixed(1);
      lastOkTs = Date.now(); updateConnVisual();
    }catch(_){}
  }
  refreshTemps(); setInterval(refreshTemps, 2000);

  // Sincronização do duty (reflete alterações externas)
  let syncBusy = false;
  async function refreshDuty(){
    if (dragging || syncBusy) return;
    try{
      syncBusy = true;
      const r = await fetch('/get_dutycicle');
      if(!r.ok) throw new Error();
      const j = await r.json();
      const raw = parseInt(j.duty || j.valor || j.raw || 0, 10);
      if (!isNaN(raw)) setVisual(raw);
      lastOkTs = Date.now(); updateConnVisual();
    }catch(_){
    }finally{
      syncBusy = false;
    }
  }
  refreshDuty(); setInterval(refreshDuty, 1500);

  // ----- Modal de Config -----
  function openModal(el){ el.style.display='flex'; }
  function closeModal(el){ el.style.display='none'; }
  btnConfig.addEventListener('click', ()=> openModal(modalConfig));
  btnCloseConfig.addEventListener('click', ()=> closeModal(modalConfig));
  modalConfig.addEventListener('click', (e)=>{ if(e.target===modalConfig) closeModal(modalConfig); });

  // Pré-carrega configs (inclui Local para o topo)
  async function preloadConfigs(){
    try{
      const r = await fetch('/recuperarConfigs');
      if(!r.ok) return;
      const txt = await r.text();
      const p = txt.split(',');
      if (p.length>=6){
        fWifi.value = p[0]||''; fSenha.value = p[1]||''; fIp.value = p[2]||'';
        fUser.value = p[3]||''; fPass.value = p[4]||''; fLocal.value = p[5]||'';
        const loc = (p[5]||'').trim();
        if (loc){
          localText.textContent = loc;
          localPill.style.display = 'inline-flex';
        } else {
          localPill.style.display = 'none';
        }
      }
    }catch(_){}
  }
  preloadConfigs();

  // Envia POST /configurar
  formConfig.addEventListener('submit', async (e)=>{
    e.preventDefault();
    const payload = [fWifi.value.trim(), fSenha.value.trim(), fIp.value.trim(), fUser.value.trim(), fPass.value.trim(), fLocal.value.trim()].join(',');
    try{
      const r = await fetch('/configurar', { method:'POST', headers:{'Content-Type':'text/plain'}, body: payload });
      if(!r.ok) throw new Error();
      toast('Configurações enviadas. O dispositivo irá reiniciar…','ok');
      setTimeout(()=>{ closeModal(modalConfig); }, 800);
    }catch(_){ toast('Falha ao enviar configurações','err'); }
  });

  /* ========= Easter Egg: segurar 7s o ícone ========= */
  const logo = document.getElementById('logoEgg');
  const egg = document.getElementById('egg');
  let eggTimer = null;

  function showEgg(){
    egg.style.display = 'flex';
    const close = ()=>{ egg.style.display='none'; egg.removeEventListener('click', onBackdrop); };
    function onBackdrop(e){ if(e.target === egg) close(); }
    egg.addEventListener('click', onBackdrop);
    setTimeout(close, 2200);
  }

  function startEggTimer(){
    if (eggTimer) clearTimeout(eggTimer);
    eggTimer = setTimeout(showEgg, 7000); // 7s segurando
  }
  function cancelEggTimer(){
    if (eggTimer){ clearTimeout(eggTimer); eggTimer=null; }
  }

  logo.addEventListener('pointerdown', startEggTimer);
  logo.addEventListener('pointerup', cancelEggTimer);
  logo.addEventListener('pointerleave', cancelEggTimer);
  logo.addEventListener('pointercancel', cancelEggTimer);

})();
</script>
</body>
</html>
)rawliteral";
