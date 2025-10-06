const TOKEN_STORAGE_KEY = 'stagecue-token';
const DEFAULT_TOKEN = 'stagecue-admin';
const MAX_LOG_ENTRIES = 50;

const state = {
  token: localStorage.getItem(TOKEN_STORAGE_KEY) || DEFAULT_TOKEN,
  ws: null,
  reconnectTimer: null,
  reconnectDelay: 2000,
  expectingClose: false,
  cues: new Map(),
};

const connectionIndicator = document.getElementById('connectionIndicator');
const connectionLabel = document.getElementById('connectionLabel');
const connectionDetails = document.getElementById('connectionDetails');
const tokenForm = document.getElementById('tokenForm');
const tokenInput = document.getElementById('tokenInput');
const cueCards = Array.from(document.querySelectorAll('.cue'));
const logList = document.getElementById('eventLog');
const clearLogButton = document.getElementById('clearLog');
const connectionPanel = document.getElementById('connectionPanel');

function applyTokenToForm() {
  tokenInput.value = state.token;
}

function setConnectionState(status, details, indicatorClass) {
  connectionPanel.dataset.status = status;
  connectionIndicator.className = `status-indicator ${indicatorClass}`;
  connectionLabel.textContent = status;
  connectionDetails.textContent = details;
}

function fetchOptions(options = {}) {
  const headers = new Headers(options.headers || {});
  if (state.token) {
    headers.set('X-StageCue-Token', state.token);
  }
  return { ...options, headers };
}

async function fetchJson(url, options) {
  const response = await fetch(url, fetchOptions(options));
  if (!response.ok) {
    const text = await response.text();
    throw new Error(`HTTP ${response.status}: ${text}`);
  }
  const contentType = response.headers.get('content-type') || '';
  if (contentType.includes('application/json')) {
    return response.json();
  }
  return response.text();
}

function logEvent(message, type = 'info') {
  const entry = document.createElement('li');
  entry.className = `log-entry log-entry--${type}`;
  entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
  logList.prepend(entry);
  while (logList.children.length > MAX_LOG_ENTRIES) {
    logList.removeChild(logList.lastChild);
  }
}

function updateCueCard(index, cueState) {
  const card = cueCards[index];
  if (!card) return;

  const textArea = card.querySelector('textarea');
  const statusLabel = card.querySelector('.cue-status');

  if (document.activeElement !== textArea) {
    textArea.value = cueState.text || '';
  }

  if (cueState.active) {
    card.classList.add('cue--active');
    statusLabel.textContent = 'Actif';
  } else {
    card.classList.remove('cue--active');
    statusLabel.textContent = 'Inactive';
  }

  card.dataset.displayReady = cueState.displayReady ? 'true' : 'false';
}

function applySnapshot(snapshot) {
  if (!snapshot || !Array.isArray(snapshot.cues)) return;
  snapshot.cues.forEach((cue) => {
    state.cues.set(cue.index, cue);
    updateCueCard(cue.index, cue);
  });
}

function handleCueUpdate(cue) {
  if (!cue || typeof cue.index === 'undefined') return;
  state.cues.set(cue.index, cue);
  updateCueCard(cue.index, cue);
  logEvent(`Cue ${cue.index + 1} → ${cue.active ? 'déclenché' : 'repos'} (${cue.text})`, 'info');
}

function buildWebSocketUrl() {
  const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
  const tokenPart = state.token ? `?token=${encodeURIComponent(state.token)}` : '';
  return `${protocol}://${window.location.host}/ws${tokenPart}`;
}

function scheduleReconnect() {
  clearTimeout(state.reconnectTimer);
  state.reconnectTimer = setTimeout(() => {
    connectWebSocket();
  }, state.reconnectDelay);
}

function connectWebSocket(manual = false) {
  clearTimeout(state.reconnectTimer);
  state.expectingClose = manual;

  const previousSocket = state.ws;
  if (previousSocket) {
    try {
      previousSocket.onclose = null;
      previousSocket.close();
    } catch (error) {
      console.warn('Erreur lors de la fermeture du WebSocket', error);
    }
  }

  const url = buildWebSocketUrl();
  const socket = new WebSocket(url);
  state.ws = socket;

  setConnectionState('Connexion en cours…', 'Ouverture du canal temps réel.', 'status-indicator--pending');

  socket.onopen = () => {
    setConnectionState('Connecté', 'Communication WebSocket sécurisée.', 'status-indicator--success');
    logEvent('Connexion WebSocket établie.');
    state.reconnectDelay = 2000;
    state.expectingClose = false;
    fetchSnapshot();
  };

  socket.onclose = (event) => {
    const reason = event.reason || 'Connexion fermée';
    setConnectionState('Déconnecté', `${reason}. Reconnexion automatique…`, 'status-indicator--error');
    logEvent(`WebSocket fermé : ${reason}`, 'warn');
    const manualClosure = state.expectingClose;
    state.expectingClose = false;
    if (!manualClosure) {
      state.reconnectDelay = Math.min(state.reconnectDelay * 1.5, 15000);
      scheduleReconnect();
    }
  };

  socket.onerror = (event) => {
    console.error('WebSocket error', event);
    setConnectionState('Erreur WebSocket', 'Vérifiez le jeton et le réseau.', 'status-indicator--error');
  };

  socket.onmessage = (event) => {
    try {
      const payload = JSON.parse(event.data);
      if (payload.type === 'snapshot') {
        applySnapshot(payload);
      } else if (payload.type === 'cue') {
        handleCueUpdate(payload);
      } else if (payload.type === 'error') {
        logEvent(`Erreur serveur : ${payload.message}`, 'error');
      }
    } catch (error) {
      console.error('Message JSON invalide', event.data, error);
    }
  };
}

async function fetchSnapshot() {
  try {
    const data = await fetchJson('/api/cues', { method: 'GET' });
    applySnapshot(data);
  } catch (error) {
    logEvent(`Impossible de récupérer l'état initial : ${error.message}`, 'error');
  }
}

async function persistCueText(index) {
  const card = cueCards[index];
  const textarea = card.querySelector('textarea');
  const text = textarea.value.trim();

  try {
    const body = new URLSearchParams({ cue: index, text });
    const response = await fetchJson('/api/cues/text', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body.toString(),
    });
    handleCueUpdate(response);
    logEvent(`Texte du cue ${index + 1} enregistré.`, 'info');
  } catch (error) {
    logEvent(`Échec d'enregistrement du cue ${index + 1} : ${error.message}`, 'error');
  }
}

function sendTrigger(index, persist = true) {
  const card = cueCards[index];
  const textarea = card.querySelector('textarea');
  const text = textarea.value.trim();

  const payload = {
    type: 'trigger',
    cue: index,
    text,
    persist,
  };

  if (state.ws && state.ws.readyState === WebSocket.OPEN) {
    state.ws.send(JSON.stringify(payload));
    logEvent(`Commande de cue ${index + 1} envoyée via WebSocket.`, 'info');
  } else {
    persistCueText(index).then(() => triggerViaHttp(index)).catch(() => triggerViaHttp(index));
  }
}

async function triggerViaHttp(index) {
  try {
    const body = new URLSearchParams({ cue: index });
    const response = await fetchJson('/api/cues/trigger', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body.toString(),
    });
    handleCueUpdate(response);
    logEvent(`Cue ${index + 1} déclenché via HTTP.`, 'info');
  } catch (error) {
    logEvent(`Impossible de déclencher le cue ${index + 1} : ${error.message}`, 'error');
  }
}

function bindCueEvents() {
  cueCards.forEach((card, index) => {
    card.addEventListener('click', (event) => {
      const button = event.target.closest('button[data-action]');
      if (!button) return;

      const action = button.dataset.action;
      if (action === 'save') {
        persistCueText(index);
      } else if (action === 'trigger') {
        sendTrigger(index);
      }
    });
  });
}

function initTokenForm() {
  applyTokenToForm();

  tokenForm.addEventListener('submit', (event) => {
    event.preventDefault();
    state.token = tokenInput.value.trim();
    localStorage.setItem(TOKEN_STORAGE_KEY, state.token);
    logEvent('Jeton mis à jour. Reconnexion en cours…', 'info');
    connectWebSocket(true);
  });
}

function initLogControls() {
  clearLogButton.addEventListener('click', () => {
    logList.innerHTML = '';
  });
}

function init() {
  bindCueEvents();
  initTokenForm();
  initLogControls();
  connectWebSocket();
}

window.addEventListener('DOMContentLoaded', init);
