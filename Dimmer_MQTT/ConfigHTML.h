#pragma once
static const char paginaConfig[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no" />
<title>DEE • Configuração</title>
<style>
  :root{
    --blue-900:#0a2540; --blue-800:#113a6b; --blue-700:#1a4d8f;
    --blue-600:#2a6fdb; --blue-500:#3b82f6; --blue-400:#60a5fa; --blue-300:#93c5fd;
    --ink-900:#0b1220;
    --bg-grad: radial-gradient(1200px 900px at 20% -10%, rgba(59,130,246,.25), transparent 60%),
               radial-gradient(1200px 900px at 120% 10%, rgba(37,99,235,.30), transparent 60%),
               linear-gradient(180deg, #0b1220 0%, #0a2540 100%);
    --glass: rgba(255,255,255,.08);
    --glass-strong: rgba(255,255,255,.14);
    --ring: rgba(59,130,246,.45);
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
    width:34px; height:34px; border-radius:10px; flex:0 0 auto;
    background:conic-gradient(from 220deg, var(--blue-300), var(--blue-600), var(--blue-400));
    display:grid; place-items:center; box-shadow:0 6px 22px rgba(59,130,246,.35) inset;
  }
  .logo svg{width:20px; height:20px; opacity:.95}
  .brand h1{margin:0; font-size:16px; font-weight:700; letter-spacing:.4px; color:#dbeafe}
  .wrap{
    flex:1; display:grid; place-items:center; padding:24px;
  }
  .card{
    width:min(840px,96vw);
    background:var(--glass); border:1px solid rgba(255,255,255,.08);
    border-radius:22px; padding:22px;
    box-shadow:0 24px 80px rgba(3,24,66,.45), inset 0 1px 0 rgba(255,255,255,.06);
  }
  .header{
    display:flex; align-items:center; justify-content:space-between; gap:12px; margin-bottom:14px;
  }
  .title{
    margin:0; font-size:18px; font-weight:800; color:#eaf2ff; letter-spacing:.4px;
  }
  .pill{
    display:inline-flex; align-items:center; gap:8px; padding:8px 12px;
    background:rgba(59,130,246,.14); border:1px solid rgba(255,255,255,.12);
    border-radius:999px; color:#dceaff; font-weight:600; letter-spacing:.2px; font-size:13px;
  }
  .form{
    display:grid; gap:14px; grid-template-columns:1fr 1fr; margin-top:8px;
  }
  .full{grid-column:1/-1}
  .field{display:flex; flex-direction:column; gap:8px}
  .field label{font-size:12px; color:#b7c6e6; letter-spacing:.25px}
  .input{
    display:flex; align-items:center; gap:8px;
    background:rgba(255,255,255,.08); border:1px solid rgba(255,255,255,.12);
    border-radius:12px; padding:12px 12px;
    transition:border-color .2s, box-shadow .2s;
  }
  .input:focus-within{
    border-color:var(--glass-strong); box-shadow:0 0 0 4px rgba(59,130,246,.15);
  }
  .input input{
    width:100%; border:0; outline:none; background:transparent; color:#e8f1ff; font-size:15px;
  }
  .input .ico{width:18px; height:18px; opacity:.9}
  .actions{display:flex; justify-content:flex-end; gap:10px; margin-top:4px}
  .btn{
    display:inline-flex; align-items:center; justify-content:center; gap:8px;
    padding:10px 14px; border-radius:12px; font-weight:800; letter-spacing:.3px; font-size:14px;
    border:1px solid rgba(255,255,255,.10); color:#0b1220; background:#93c5fd;
    box-shadow:0 10px 20px rgba(59,130,246,.35); cursor:pointer; transition:.18s transform,.18s filter;
  }
  .btn:hover{transform:translateY(-1px); filter:saturate(1.05)}
  .hint{color:#9db7e7; font-size:12px; margin-top:4px}
  /* Toast único, embaixo */
  .status{position:fixed; left:50%; transform:translateX(-50%); bottom:14px; z-index:60; width: min(92vw, 520px);
    display:flex; justify-content:center; pointer-events:none}
  .toast{
    background:rgba(10,37,64,.90); border:1px solid rgba(255,255,255,.10); color:#e6f0ff;
    border-radius:12px; padding:10px 12px; font-size:13px; letter-spacing:.2px;
    box-shadow:0 14px 40px rgba(0,0,0,.35); display:flex; align-items:center; gap:10px; max-width:100%; pointer-events:auto
  }
  .toast.ok{border-color:rgba(34,197,94,.45)} .toast.err{border-color:rgba(239,68,68,.45)}
  @media (max-width:720px){ .form{grid-template-columns:1fr} .wrap{padding:16px} .card{padding:16px} }
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
      <h1>DEE • Configuração</h1>
    </div>
  </header>

  <main class="wrap">
    <section class="card">
      <div class="header">
        <h2 class="title">Parâmetros de Rede & MQTT</h2>
        <span class="pill">Dispositivo DEE</span>
      </div>

      <form id="formConfig" class="form">
        <div class="field">
          <label>Wi-Fi (SSID)</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M2 8.82A15.94 15.94 0 0 1 12 5c3.7 0 7.09 1.34 9.74 3.56"/><path d="M5 12.55A11.94 11.94 0 0 1 12 10c2.67 0 5.13.93 7.06 2.48"/><path d="M8.5 16.08A7.94 7.94 0 0 1 12 15c1.63 0 3.15.49 4.41 1.33"/><circle cx="12" cy="19" r="1"/></svg>
            <input id="f_wifi" placeholder="Nome da rede" required />
          </div>
        </div>

        <div class="field">
          <label>Senha Wi-Fi</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><rect x="3" y="11" width="18" height="10" rx="2"/><path d="M7 11V7a5 5 0 1 1 10 0v4"/></svg>
            <input id="f_senha" type="password" placeholder="********" />
          </div>
        </div>

        <div class="field">
          <label>IP do Broker</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><rect x="3" y="4" width="18" height="12" rx="2"/><path d="M7 20h10"/><path d="M12 16v4"/></svg>
            <input id="f_ip" placeholder="192.168.0.10" required />
          </div>
        </div>

        <div class="field">
          <label>Usuário MQTT</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M20 21v-2a4 4 0 0 0-3-3.87"/><path d="M4 21v-2a4 4 0 0 1 3-3.87"/><circle cx="12" cy="7" r="4"/></svg>
            <input id="f_user" placeholder="(opcional)" />
          </div>
        </div>

        <div class="field">
          <label>Senha MQTT</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><circle cx="12" cy="5" r="2"/><path d="M12 7v7"/><path d="M10 11h4"/><path d="M5 19h14"/></svg>
            <input id="f_pass" type="password" placeholder="(opcional)" />
          </div>
        </div>

        <div class="field full">
          <label>Local</label>
          <div class="input">
            <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M21 10c0 7-9 12-9 12S3 17 3 10a9 9 0 1 1 18 0Z"/><circle cx="12" cy="10" r="3"/></svg>
            <input id="f_local" placeholder="Ex.: Linha 1 • Ventilador A" />
          </div>
        </div>

        <div class="actions full">
          <button type="submit" class="btn">Salvar & Reiniciar</button>
        </div>
      </form>

      <div class="hint">As configurações são aplicadas imediatamente após o envio.</div>
    </section>
  </main>

  <!-- Toast único -->
  <div class="status" id="status"></div>

<script>
(function(){
  const statusBox = document.getElementById('status');
  const form = document.getElementById('formConfig');

  const fWifi  = document.getElementById('f_wifi');
  const fSenha = document.getElementById('f_senha');
  const fIp    = document.getElementById('f_ip');
  const fUser  = document.getElementById('f_user');
  const fPass  = document.getElementById('f_pass');
  const fLocal = document.getElementById('f_local');

  function toast(msg, kind='ok'){
    statusBox.innerHTML = '';
    const el = document.createElement('div');
    el.className = 'toast '+(kind==='ok'?'ok':'err');
    el.textContent = msg;
    statusBox.appendChild(el);
    setTimeout(()=>{ el.style.opacity='0'; el.style.transform='translateY(4px)'; }, 1800);
    setTimeout(()=>{ el.remove(); }, 2300);
  }

  // Pré-carrega configs
  async function preload(){
    try{
      const r = await fetch('/recuperarConfigs');
      if(!r.ok) return;
      const t = await r.text();
      const p = t.split(',');
      if (p.length>=6){
        fWifi.value  = p[0]||'';
        fSenha.value = p[1]||'';
        fIp.value    = p[2]||'';
        fUser.value  = p[3]||'';
        fPass.value  = p[4]||'';
        fLocal.value = p[5]||'';
      }
    }catch(_){}
  }
  preload();

  // Envia POST /configurar com CSV simples no corpo
  form.addEventListener('submit', async (e)=>{
    e.preventDefault();
    const payload = [
      fWifi.value.trim(),
      fSenha.value.trim(),
      fIp.value.trim(),
      fUser.value.trim(),
      fPass.value.trim(),
      fLocal.value.trim()
    ].join(',');
    try{
      const r = await fetch('/configurar', {
        method:'POST',
        headers:{'Content-Type':'text/plain'},
        body: payload
      });
      if(!r.ok) throw new Error();
      toast('Configurações enviadas. Reiniciando…','ok');
      // reinício é feito pelo firmware após responder OK
    }catch(_){
      toast('Falha ao enviar configurações','err');
    }
  });
})();
</script>
</body>
</html>
)rawliteral";
