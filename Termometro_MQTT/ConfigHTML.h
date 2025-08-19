#pragma once
static const char paginaConfig[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no" />
<title>DEE • Configuração do Termômetro</title>
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
    position:sticky; top:0; z-index:50; display:flex; align-items:center; justify-content:center;
    padding:12px 16px; backdrop-filter: blur(10px) saturate(140%);
    background:linear-gradient(180deg, rgba(10,37,64,.9), rgba(10,37,64,.55));
    border-bottom:1px solid rgba(255,255,255,.06);
  }
  .brand{display:flex; align-items:center; gap:10px}
  .logo{width:34px;height:34px;border-radius:10px;display:grid;place-items:center;
    background:conic-gradient(from 220deg, var(--blue-300), var(--blue-600), var(--blue-400));
    box-shadow:inset 0 6px 22px rgba(59,130,246,.35)}
  .brand h1{margin:0;font-size:16px;font-weight:800;color:#dbeafe}
  .wrap{flex:1;display:grid;place-items:center;padding:20px}
  .card{width:min(700px,96vw);background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.08);
    border-radius:20px;padding:20px;box-shadow:0 24px 80px rgba(3,24,66,.45), inset 0 1px 0 rgba(255,255,255,.06)}
  .form{display:grid;gap:12px;grid-template-columns:1fr 1fr}
  .full{grid-column:1/-1}
  .field{display:flex;flex-direction:column;gap:6px}
  .field label{font-size:12px;color:#b7c6e6}
  .field input{padding:12px;border-radius:10px;border:1px solid rgba(255,255,255,.12);background:rgba(255,255,255,.08);color:#eaf2ff;outline:none;font-size:15px}
  .actions{display:flex;justify-content:flex-end;gap:10px;margin-top:6px}
  .btn{display:inline-flex;align-items:center;gap:8px;padding:10px 14px;border-radius:12px;font-weight:800;letter-spacing:.3px;font-size:14px;
    border:1px solid rgba(255,255,255,.10);color:#0b1220;background:#93c5fd;box-shadow:0 10px 20px rgba(59,130,246,.35);cursor:pointer}
  @media (max-width:640px){ .form{grid-template-columns:1fr} .wrap{padding:14px} .card{padding:14px} }
</style>
</head>
<body>
  <header class="topbar">
    <div class="brand">
      <div class="logo" aria-hidden="true">
        <svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 2v6M12 16v6M4 12h6M14 12h6" />
          <circle cx="12" cy="12" r="5"></circle>
        </svg>
      </div>
      <h1>DEE • Configuração do Termômetro</h1>
    </div>
  </header>

  <main class="wrap">
    <section class="card">
      <form id="configForm" class="form">
        <div class="field"><label>Wi-Fi (SSID)</label><input id="ssid" placeholder="Nome da rede" required /></div>
        <div class="field"><label>Senha Wi-Fi</label><input id="wifiPass" type="password" placeholder="********" /></div>
        <div class="field"><label>Host Broker MQTT</label><input id="mqttHost" placeholder="192.168.0.10" required /></div>
        <div class="field"><label>Usuário MQTT</label><input id="mqttUser" placeholder="(opcional)" /></div>
        <div class="field"><label>Senha MQTT</label><input id="mqttPass" type="password" placeholder="(opcional)" /></div>
        <div class="field"><label>Tópico de publicação</label><input id="topic" placeholder="sensores/temperatura/interna" /></div>
        <div class="field full"><label>Local</label><input id="local" placeholder="Ex.: Externo" /></div>
        <div class="actions full"><button type="submit" class="btn">Salvar & Reiniciar</button></div>
      </form>
    </section>
  </main>

<script>
(function(){
  const $ = id => document.getElementById(id);

  async function preload(){
    try{
      const r = await fetch('/recuperarConfigs');
      if(!r.ok) return;
      const csv = await r.text();               // wifi,senha,ip,user,pass,topic,local
      const p = csv.split(',');
      $('ssid').value     = p[0]||'';
      $('wifiPass').value = p[1]||'';
      $('mqttHost').value = p[2]||'';
      $('mqttUser').value = p[3]||'';
      $('mqttPass').value = p[4]||'';
      $('topic').value    = p[5]||'sensores/temperatura/interna';
      $('local').value    = p[6]||'';
    }catch(_){}
  }
  preload();

  $('configForm').addEventListener('submit', async (e)=>{
    e.preventDefault();
    const payload = [
      $('ssid').value.trim(),
      $('wifiPass').value.trim(),
      $('mqttHost').value.trim(),
      $('mqttUser').value.trim(),
      $('mqttPass').value.trim(),
      $('topic').value.trim(),
      $('local').value.trim()
    ].join(',');
    try{
      const r = await fetch('/configurar', { method:'POST', headers:{'Content-Type':'text/plain'}, body: payload });
      if(!r.ok) throw new Error();
      alert('Configurações salvas. O dispositivo irá reiniciar.');
    }catch(_){
      alert('Falha ao salvar configurações.');
    }
  });
})();
</script>
</body>
</html>
)rawliteral";
