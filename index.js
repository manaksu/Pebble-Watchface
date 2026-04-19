/* PebbleKit JS — Literary Watchface */

var CONFIG_URL = 'https://manaksu.github.io/Pebble-Watchface/config.html';

function send(w, f) {
  var msg = {};
  msg[1] = w;
  msg[2] = f;
  Pebble.sendAppMessage(msg,
    function() { console.log('sent w='+w+' f='+f); },
    function(e) { console.log('fail: '+JSON.stringify(e)); }
  );
}

Pebble.addEventListener('ready', function() {
  send(
    parseInt(localStorage.getItem('widget')||'3',10),
    parseInt(localStorage.getItem('font')||'1',10)
  );
});

Pebble.addEventListener('showConfiguration', function() {
  var w = localStorage.getItem('widget')||'3';
  var f = localStorage.getItem('font')||'1';
  Pebble.openURL(CONFIG_URL + '?w=' + w + '&f=' + f);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e||!e.response||e.response===''||e.response==='CANCELLED') return;
  var raw = e.response;
  if (raw.charAt(0)==='#') raw = raw.slice(1);
  var cfg;
  try { cfg=JSON.parse(decodeURIComponent(raw)); } catch(err) { return; }
  if (cfg.widget!==undefined) localStorage.setItem('widget',''+cfg.widget);
  if (cfg.font!==undefined)   localStorage.setItem('font',''+cfg.font);
  send(
    parseInt(localStorage.getItem('widget'),10),
    parseInt(localStorage.getItem('font'),10)
  );
});
