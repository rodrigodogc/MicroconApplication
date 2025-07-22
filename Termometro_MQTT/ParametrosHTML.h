static const char paginaParametros[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Temperaturas DEE</title>
  <style>
    :root {
      --primary-color: #005a9c;  /* Azul UFPE */
      --secondary-color: #f0f2f5;
      --text-color: #333;
      --border-radius: 8px;
    }
    html {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    }
    body {
      margin: 0;
      padding-top: 80px;  
      background-color: var(--secondary-color);
      color: var(--text-color);
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    nav {
      display: flex;
      align-items: center;
      justify-content: space-between; 
      background: #fff;
      padding: 0 2rem;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 70px;
      z-index: 1000;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
      box-sizing: border-box;
    }
    .logo {
      height: 56px;
      width: auto;
      margin-right: 1.2rem;
    }
    .nav-title {
      font-size: 1.4rem;
      font-weight: 600;
      color: var(--primary-color);
      margin-right: 2rem;
      white-space: nowrap;
    }
    .nav-buttons button {
      background: none;
      color: var(--text-color);
      border: none;
      padding: 1rem 0.8rem;
      margin: 0 0.2rem;
      cursor: pointer;
      font-size: 1rem;
      font-weight: 500;
      border-bottom: 3px solid transparent;
      transition: all 0.3s ease;
    }
    .nav-buttons button:hover {
      color: var(--primary-color);
    }
    .nav-buttons button.active {
      color: var(--primary-color);
      border-bottom: 3px solid var(--primary-color);
    }
    .section-box {
      background: #fff;
      border-radius: var(--border-radius);
      box-shadow: 0 4px 12px rgba(0,0,0,0.08);
      padding: 2rem;
      margin-top: 2rem;
      max-width: 500px;
      width: 90%;
      box-sizing: border-box;
      text-align: center;
    }
    .hidden { display: none; }
    h2 { margin-top: 0; color: var(--primary-color); }
    .gauge-container {
      width: 420px;
      height: 340px;
      margin: 2rem auto 0 auto;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .justgage .value {      /* aumenta a fonte do valor da temperatura */
      font-size: 4.2rem !important;
      font-weight: bold;
    }
    .config-form {
      display: flex;
      flex-direction: column;
      text-align: left;
    }
    .form-group { margin-bottom: 1rem; }
    .form-group label {
      display: block;
      margin-bottom: 0.3rem;
      font-weight: 500;
    }
    .form-group input {
      width: 100%;
      padding: 0.7rem;
      border: 1px solid #ccc;
      border-radius: var(--border-radius);
      font-size: 1rem;
      box-sizing: border-box;
    }
    .form-group input:focus {
      outline: none;
      border-color: var(--primary-color);
      box-shadow: 0 0 5px rgba(0, 90, 156, 0.3);
    }
    .config-form button {
      padding: 0.8rem;
      font-size: 1rem;
      background: var(--primary-color);
      color: #fff;
      border: none;
      border-radius: var(--border-radius);
      cursor: pointer;
      width: 100%;
    }
    #statusMsg { margin-top: 1rem; font-size: .9rem; }
  </style>
</head>
<body>
  <nav>
    <div style="display: flex; align-items: center;">
      <img src="https://upload.wikimedia.org/wikipedia/commons/8/85/Bras%C3%A3o_da_UFPE.png" alt="Logo UFPE" class="logo">
      <span class="nav-title">Monitor de temperatura (<span id="navLocal">Local</span>)</span>
    </div>
    <div class="nav-buttons">
      <button id="btnMonitor" class="active">Monitoramento</button>
      <button id="btnConfig">Configurações</button>
    </div>
  </nav>

  <div id="monitorBox" class="section-box">
    <h2>Monitoramento de Temperatura</h2>
    <div id="gauge" class="gauge-container"></div>
  </div>

  <div id="configBox" class="section-box hidden">
    <h2>Configuração Wi-Fi / MQTT</h2>
    <form class="config-form" id="configForm">
      <div class="form-group">
        <label for="ssid">SSID Wi-Fi</label>
        <input type="text" id="ssid" placeholder="Nome da rede">
      </div>
      <div class="form-group">
        <label for="wifiPass">Senha Wi-Fi</label>
        <input type="password" id="wifiPass" placeholder="Senha da rede">
      </div>
      <div class="form-group">
        <label for="mqttHost">Host Broker MQTT</label>
        <input type="text" id="mqttHost" placeholder="IP ou domínio">
      </div>
      <div class="form-group">
        <label for="mqttUser">Usuário MQTT</label>
        <input type="text" id="mqttUser" placeholder="Usuário">
      </div>
      <div class="form-group">
        <label for="mqttPass">Senha MQTT</label>
        <input type="password" id="mqttPass" placeholder="Senha">
      </div>
      <div class="form-group">
        <label for="local">Local</label>
        <input type="text" id="local" placeholder="Ex: Laboratório 1">
      </div>
      <button type="submit">Salvar Configurações</button>
      <div id="statusMsg"></div>
    </form>
  </div>

  <!-- bibliotecas necessárias pro gauge -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.3.0/raphael.min.js"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/justgage/1.6.1/justgage.min.js"></script>

  <script>
    const btnMonitor = document.getElementById('btnMonitor');
    const btnConfig  = document.getElementById('btnConfig');
    const monitorBox = document.getElementById('monitorBox');
    const configBox  = document.getElementById('configBox');
    const navLocal   = document.getElementById('navLocal');

    /* Instancia o gauge */
    const gauge = new JustGage({
      id: 'gauge',
      value: 0,
      min: 0,
      max: 50,
      title: 'Temperatura',
      label: '°C',
      levelColors: ['#007bff', '#f9c851', '#ff4500'],
      pointer: true,
      pointerOptions: { color: '#333' },
      gaugeWidthScale: 0.6,
      decimals: 2,
      counter: true
    });

    const $ = id => document.getElementById(id);
    const status = msg => { $('statusMsg').innerText = msg; };

    /* Lê configuração salva ao carregar a página e começa a atualizar a temperatura*/
    document.addEventListener('DOMContentLoaded', async () => {
      status('Carregando configuração...');
      try {
        const r = await fetch('/recuperarConfigs');
        if (!r.ok) throw new Error(await r.text());
        const csv = await r.text();
        // preenche o formulário com os dadosda esp quando a pag carrega
        const [ssid,pwd,ip,user,pass,local] = csv.split(',');
          $('ssid').value      = ssid  ?? '';
          $('wifiPass').value  = pwd   ?? '';
          $('mqttHost').value  = ip    ?? '';
          $('mqttUser').value  = user  ?? '';
          $('mqttPass').value  = pass  ?? '';
          $('local').value     = local ?? '';
          navLocal.textContent = local || 'Local';
        status('Configuração carregada.');
      } catch {
        status('Sem configuração salva.');
      }

      const atualizarTemp = async () => {
        const response = await fetch('/get_temperatura');
        const data = await response.json();
        console.log(data);
        const temp = parseFloat(data.temp);
        if (!isNaN(temp)) {
          gauge.refresh(temp);
          console.log('Temperatura atualizada: ' + temp);
        }
        return true;
      }

      setInterval(() => {
        atualizarTemp();
      }, 4000);
    });

    /* Navegação */
    function switchView(view) {
      if (view === 'monitor') {
        btnMonitor.classList.add('active');
        btnConfig .classList.remove('active');
        monitorBox.classList.remove('hidden');
        configBox .classList.add   ('hidden');
      } else {
        btnConfig .classList.add   ('active');
        btnMonitor.classList.remove('active');
        configBox .classList.remove('hidden');
        monitorBox.classList.add   ('hidden');
      }
    }
    btnMonitor.addEventListener('click', () => switchView('monitor'));
    btnConfig .addEventListener('click', () => switchView('config' ));

    /* Envia CSV para /configurar */
    document.getElementById('configForm').addEventListener('submit', async e => {
      e.preventDefault();
      const csv = [
        $('ssid').value,
        $('wifiPass').value,
        $('mqttHost').value,
        $('mqttUser').value,
        $('mqttPass').value,
        $('local').value
      ].join(',');

      status('Enviando configurações...');
      try {
        const r = await fetch('/configurar', {
          method : 'POST',
          headers: { 'Content-Type': 'text/plain' },
          body   : csv
        });
        if (!r.ok) throw new Error(await r.text());
        localStorage.setItem('local', $('local').value);
        navLocal.textContent = $('local').value || 'Local';
        status('Configurações salvas! Reiniciando dispositivo...');
        alert('Configurações salvas. O dispositivo vai reiniciar.');
      } catch (err) {
        status(`Erro ao salvar: ${err.message}`);
      }
    });
  </script>
</body>
</html>

)rawliteral";