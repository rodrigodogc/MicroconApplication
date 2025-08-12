

fetch("http://192.168.0.109/set_dutycicle", {
  method: "POST",
  headers: {
      "Content-Type": "application/x-www-form-urlencoded"
  },
  body: "dutycicle=120"
});

