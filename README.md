# ttyhook

A proof-of-concept prototype for hooking into Direwolf's serial PTT RTS/DTR signal in order
to run arbitrary actions before/after the PTT is toggled. It currently doesn't differentiate
between which serial port Direwolf is toggling the PTT on so if you use multiple be aware of
that limitation.
