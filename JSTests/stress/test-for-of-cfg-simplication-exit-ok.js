let z = {}
z.__proto__ = []
for (let i = 0; i < testLoopCount; i++) {
  for (let x of ['', z]) {
    for (let y of x) {}
  }
}
