/* PebbleKit JS — Literary Watchface */

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
  
    /*
     * Build URL using only safe ASCII — no encodeURIComponent.
     * Pebble's webview handles plain data:text/html with unencoded HTML
     * better than a fully percent-encoded string.
     * Only # and spaces need encoding in the HTML content itself.
     */
    var url = 'data:text/html,'
      + '<html><body style="background:white;padding:16px;font-family:sans-serif">'
      + '<h2>Literary Settings</h2>'
      + '<p><b>Widget</b></p>'
      + '<p><label><input type=radio name=w value=0' + (w==='0'?' checked':'') + '> Sine Wave</label></p>'
      + '<p><label><input type=radio name=w value=1' + (w==='1'?' checked':'') + '> Bar Chart</label></p>'
      + '<p><label><input type=radio name=w value=2' + (w==='2'?' checked':'') + '> Battery Line</label></p>'
      + '<p><label><input type=radio name=w value=3' + (w==='3'?' checked':'') + '> Dot ECG</label></p>'
      + '<p><b>Font</b></p>'
      + '<p><label><input type=radio name=f value=0' + (f==='0'?' checked':'') + '> Compact</label></p>'
      + '<p><label><input type=radio name=f value=1' + (f==='1'?' checked':'') + '> Standard</label></p>'
      + '<p><label><input type=radio name=f value=2' + (f==='2'?' checked':'') + '> Bold</label></p>'
      + '<p><label><input type=radio name=f value=3' + (f==='3'?' checked':'') + '> Large</label></p>'
      + '<p><button style="padding:10px 20px;font-size:16px" onclick="'
      + "var w=document.querySelector('input[name=w]:checked').value;"
      + "var f=document.querySelector('input[name=f]:checked').value;"
      + "location.href='pebblejs://close%23'+encodeURIComponent(JSON.stringify({widget:+w,font:+f}));"
      + '">Save</button></p>'
      + '</body></html>';
  
    Pebble.openURL(url);
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