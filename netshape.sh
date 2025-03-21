#!/bin/bash
# https://serverfault.com/questions/725030/traffic-shaping-on-osx-10-10-with-pfctl-and-dnctl

# Reset dummynet to default config
dnctl -f flush

# Compose an addendum to the default config: creates a new anchor
(cat /etc/pf.conf &&
  echo 'dummynet-anchor "netshape"') | pfctl -q -f -

# Configure the new anchor
cat <<EOF | pfctl -q -a netshape -f -
dummynet in proto udp from any to any port 53473 pipe 1
dummynet out proto udp from any to any port 53473 pipe 1
EOF

# Create the dummynet queue
dnctl pipe 1 config delay 150 plr 0.2 bw 50000byte/s

# Activate PF
pfctl -E

# to check that dnctl is properly configured: sudo dnctl list
