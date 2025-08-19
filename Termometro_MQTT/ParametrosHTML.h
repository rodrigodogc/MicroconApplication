#pragma once
static const char paginaParametros[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no" />
<title>DEE • Monitor de Temperatura</title>
<style>
  :root{
    --blue-900:#0a2540; --blue-800:#113a6b; --blue-700:#1a4d8f;
    --blue-600:#2a6fdb; --blue-500:#3b82f6; --blue-400:#60a5fa; --blue-300:#93c5fd;
    --bg: radial-gradient(1200px 900px at 20% -10%, rgba(59,130,246,.25), transparent 60%),
          radial-gradient(1200px 900px at 120% 10%, rgba(37,99,235,.30), transparent 60%),
          linear-gradient(180deg, #0b1220 0%, #0a2540 100%);
  }
  *{box-sizing:border-box; -webkit-tap-highlight-color:transparent}
  body{
    margin:0; background:var(--bg); color:#e8eefc;
    font:15px/1.45 ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Ubuntu;
    min-height:100vh; display:flex; flex-direction:column;
  }
  .topbar{
    position:sticky; top:0; z-index:50; display:flex; align-items:center; justify-content:space-between; gap:10px;
    padding:12px 16px; backdrop-filter: blur(10px) saturate(140%);
    background:linear-gradient(180deg, rgba(10,37,64,.9), rgba(10,37,64,.55));
    border-bottom:1px solid rgba(255,255,255,.06);
  }
  .brand{display:flex; align-items:center; gap:10px}
  .logo{width:34px;height:34px;border-radius:10px;display:grid;place-items:center;
    background:conic-gradient(from 220deg, var(--blue-300), var(--blue-600), var(--blue-400));
    box-shadow:inset 0 6px 22px rgba(59,130,246,.35)}
  .brand h1{margin:0;font-size:16px;font-weight:800;color:#dbeafe}
  .rightbar{display:flex;align-items:center;gap:10px}
  .conn{display:inline-flex;align-items:center;gap:8px;padding:8px 10px;border-radius:999px;font-weight:700;
    color:#dceaff;font-size:13px;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12)}
  .conn .dot{width:10px;height:10px;border-radius:50%;background:#94a3b8}
  .conn.online .dot{background:#22c55e;animation:pulse 1.6s infinite ease-in-out}
  @keyframes pulse{0%,100%{transform:scale(1);box-shadow:0 0 0 0 rgba(34,197,94,.45)}50%{transform:scale(1.15);box-shadow:0 0 0 6px rgba(34,197,94,.05)}}
  .ghost{display:inline-flex;align-items:center;gap:8px;padding:10px 12px;border-radius:12px;font-weight:700;
    border:1px solid rgba(255,255,255,.12);color:#dbeafe;background:rgba(59,130,246,.12);cursor:pointer}
  .wrap{flex:1;display:grid;place-items:center;padding:20px}
  .card{width:min(920px,96vw);background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.08);
    border-radius:20px;padding:20px;box-shadow:0 24px 80px rgba(3,24,66,.45), inset 0 1px 0 rgba(255,255,255,.06)}
  .title{display:flex;align-items:center;justify-content:space-between;margin:0 0 10px;font-size:15px;color:#cfe4ff;font-weight:800}
  .grid{display:grid;gap:18px;grid-template-columns:1fr}
  .box{background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.08);border-radius:16px;padding:14px;text-align:center}
  .pill{display:inline-flex;align-items:center;gap:8px;padding:7px 10px;border-radius:999px;border:1px solid rgba(255,255,255,.12);background:rgba(59,130,246,.14);font-weight:700}
  #gaugeWrap{display:flex;align-items:center;justify-content:center;min-height:320px}
  #topicLabel{color:#bad6ff;font-weight:700;margin-top:8px}
  .status{position:fixed;left:50%;transform:translateX(-50%);bottom:14px;z-index:60;width:min(92vw,520px);display:flex;justify-content:center;pointer-events:none}
  .toast{background:rgba(10,37,64,.90);border:1px solid rgba(255,255,255,.10);color:#e6f0ff;border-radius:12px;padding:10px 12px;font-size:13px;box-shadow:0 14px 40px rgba(0,0,0,.35)}
  /* Modal */
  .modal-backdrop{position:fixed;inset:0;background:rgba(3,8,20,.6);backdrop-filter:blur(4px);display:none;align-items:center;justify-content:center;z-index:80}
  .modal{width:min(560px,92vw);background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.10);border-radius:16px;padding:14px}
  .modal h3{margin:0 0 10px;font-size:18px;color:#e6f0ff}
  .fgrid{display:grid;gap:10px;grid-template-columns:1fr 1fr}
  .fgrid .full{grid-column:1/-1}
  .field{display:flex;flex-direction:column;gap:6px}
  .field label{font-size:12px;color:#b7c6e6}
  .field input{padding:12px;border-radius:10px;border:1px solid rgba(255,255,255,.12);background:rgba(255,255,255,.08);color:#eaf2ff;outline:none;font-size:15px}
  .modal-actions{display:flex;justify-content:flex-end;gap:10px;margin-top:10px}
  @media (max-width:560px){ .fgrid{grid-template-columns:1fr} .wrap{padding:14px} .card{padding:14px} }
</style>
</head>
<body>
  <header class="topbar">
    <div class="brand">
      <div class="logo" aria-hidden="true">
        <svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 2v6M12 16v6M4 12h6M14 12h6"/><circle cx="12" cy="12" r="5"></circle>
        </svg>
      </div>
      <h1>DEE • Monitor de Temperatura</h1>
    </div>
    <div class="rightbar">
      <div id="connTag" class="conn"><span class="dot"></span><span id="connText">Offline</span></div>
      <button id="btnConfig" class="ghost">Configurar</button>
    </div>
  </header>

  <main class="wrap">
    <section class="card">
      <h2 class="title">Temperatura</h2>
      <div class="grid">
        <div class="box">
          <div id="gaugeWrap"><div id="gauge"></div></div>
          <div id="topicLabel">Tópico: <strong id="topicVal">--</strong></div>
          <div class="pill" style="margin-top:10px;">Local: <strong id="locVal" style="margin-left:6px;">--</strong></div>
        </div>
      </div>
    </section>
  </main>

  <div class="status" id="status"></div>

  <!-- Modal Config -->
  <div class="modal-backdrop" id="modalConfig">
    <div class="modal">
      <h3>Configurações de Rede e MQTT</h3>
      <form id="formConfig">
        <div class="fgrid">
          <div class="field"><label>Wi-Fi (SSID)</label><input id="f_wifi" placeholder="Nome da rede" required /></div>
          <div class="field"><label>Senha Wi-Fi</label><input id="f_senha" type="password" placeholder="********" /></div>
          <div class="field"><label>IP do Broker</label><input id="f_ip" placeholder="192.168.0.10" required /></div>
          <div class="field"><label>Usuário MQTT</label><input id="f_user" placeholder="(opcional)" /></div>
          <div class="field"><label>Senha MQTT</label><input id="f_pass" type="password" placeholder="(opcional)" /></div>
          <div class="field"><label>Tópico de publicação</label><input id="f_topic" placeholder="sensores/temperatura/interna" /></div>
          <div class="field full"><label>Local</label><input id="f_local" placeholder="Ex.: Linha 1 • Forno A" /></div>
        </div>
        <div class="modal-actions">
          <button type="button" class="ghost" id="btnCloseConfig">Cancelar</button>
          <button type="submit" class="ghost">Salvar & Enviar</button>
        </div>
      </form>
    </div>
  </div>

  <script src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.3.0/raphael.min.js"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/justgage/1.6.1/justgage.min.js"></script>

<script>
(function(){
  const statusBox = document.getElementById('status');
  const connTag   = document.getElementById('connTag');
  const connText  = document.getElementById('connText');
  const topicVal  = document.getElementById('topicVal');
  const locVal    = document.getElementById('locVal');

  const btnConfig = document.getElementById('btnConfig');
  const modalConfig = document.getElementById('modalConfig');
  const btnCloseConfig = document.getElementById('btnCloseConfig');
  const fWifi  = document.getElementById('f_wifi');
  const fSenha = document.getElementById('f_senha');
  const fIp    = document.getElementById('f_ip');
  const fUser  = document.getElementById('f_user');
  const fPass  = document.getElementById('f_pass');
  const fTopic = document.getElementById('f_topic');
  const fLocal = document.getElementById('f_local');

  let lastOkTs = 0;

  function updateConnVisual(){
    const online = (Date.now() - lastOkTs) < 6000;
    connTag.classList.toggle('online', online);
    connText.textContent = online ? 'Conectado' : 'Offline';
  }
  setInterval(updateConnVisual, 1000);

  function toast(msg){
    statusBox.innerHTML = '<div class="toast">'+msg+'</div>';
    setTimeout(()=>{ statusBox.innerHTML=''; }, 2200);
  }

  function openModal(){ modalConfig.style.display='flex'; }
  function closeModal(){ modalConfig.style.display='none'; }
  btnConfig.addEventListener('click', openModal);
  btnCloseConfig.addEventListener('click', closeModal);
  modalConfig.addEventListener('click', (e)=>{ if(e.target===modalConfig) closeModal(); });

  // Gauge
  const gauge = new JustGage({
    id: 'gauge', value: 0, min: -10, max: 60,
    title: 'Temperatura', label: '°C',
    pointer: true, pointerOptions: { color: '#e8eefc' },
    levelColors: ['#60a5fa','#facc15','#f87171'],
    gaugeWidthScale: 0.6, decimals: 1, counter: true
  });

  async function preloadConfigs(){
    try{
      const r = await fetch('/recuperarConfigs');
      if(!r.ok) throw new Error();
      const csv = await r.text();
      const parts = csv.split(','); // wifi,senha,ip,user,pass,topic,local
      fWifi.value  = parts[0]||'';
      fSenha.value = parts[1]||'';
      fIp.value    = parts[2]||'';
      fUser.value  = parts[3]||'';
      fPass.value  = parts[4]||'';
      fTopic.value = parts[5]||'sensores/temperatura/interna';
      fLocal.value = parts[6]||'';
      topicVal.textContent = fTopic.value || '--';
      locVal.textContent   = fLocal.value || '--';
    }catch(_){}
  }
  preloadConfigs();

  async function refreshTemp(){
    try{
      const r = await fetch('/get_temperatura');
      if(!r.ok) throw new Error();
      const j = await r.json();
      const t = parseFloat(j.temp);
      if (!isNaN(t)) gauge.refresh(t);
      lastOkTs = Date.now(); updateConnVisual();
    }catch(_){}
  }
  refreshTemp(); setInterval(refreshTemp, 2000);

  document.getElementById('formConfig').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const payload = [
      fWifi.value.trim(), fSenha.value.trim(), fIp.value.trim(),
      fUser.value.trim(), fPass.value.trim(), fTopic.value.trim(), fLocal.value.trim()
    ].join(',');
    try{
      const r = await fetch('/configurar', { method:'POST', headers:{'Content-Type':'text/plain'}, body: payload });
      if(!r.ok) throw new Error();
      toast('Configurações enviadas. O dispositivo irá reiniciar…');
      setTimeout(closeModal, 800);
    }catch(_){ toast('Falha ao enviar configurações'); }
  });
})();
</script>
</body>
</html>
)rawliteral";
