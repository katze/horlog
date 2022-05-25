async function moveNeedle(needle, steps) {

    fetch("moveNeedle?needle="+needle+"&steps="+steps)
    .then(
      function(response) {
        if (response.status !== 200) {
          alert('Looks like there was a problem. Status Code: ' + response.status);
          return;
        }
  
        response.json().then(function(data) {
            //responseEl = document.getElementById("response");
            //responseEl.style.display = 'block';
            //messageEl = document.getElementById("message");
            //messageEl.innerHTML = data.message;
            console.log(data);
        });
      }
    )
    .catch(function(err) {
        responseEl = document.getElementById("response");
        responseEl.style.display = 'block';
        messageEl = document.getElementById("message");
        messageEl.innerHTML = "Error: "+err;
    });
}


async function validate() {

    var now = new Date();
    var timestamp = Math.floor((new Date()).getTime() / 1000);
    var offset = now.getTimezoneOffset();


    fetch("setTime?t="+timestamp+"&o="+offset)
    .then(
      function(response) {
        if (response.status !== 200) {
          alert('Looks like there was a problem. Status Code: ' + response.status);
          return;
        }
  
        response.json().then(function(data) {
            //responseEl = document.getElementById("response");
            //responseEl.style.display = 'block';
            //messageEl = document.getElementById("message");
            //messageEl.innerHTML = data.message;
            alert("La configuration est terminée, le wifi va être coupé.");
            console.log(data);
        });
      }
    )
    .catch(function(err) {
        responseEl = document.getElementById("response");
        responseEl.style.display = 'block';
        messageEl = document.getElementById("message");
        messageEl.innerHTML = "Error: "+err;
    });
}