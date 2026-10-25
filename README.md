# Zephyr Industries Load Balancer {#mainpage}

Project 4: a C++ simulation of a load balancer that feeds a queue of web requests to a pool of web servers
and grows or shrinks that pool to keep the queue between **50 and 80 requests per server**.

## Build and run

    make                      # builds ./loadbalancer
    ./loadbalancer            # asks for number of servers and cycles
    ./loadbalancer -s 20 -t 100000 -v     # 20 servers, 100000 cycles, verbose log
    ./loadbalancer -s 20 -t 100000 -q     # quiet: starting and ending summary only
    ./loadbalancer -s 20 -t 100000 -v --switch   # bonus: Switch with Streaming + Processing balancers
    make docs                 # Doxygen HTML in docs/html

## Options

| Option | Meaning |
|---|---|
| `-c FILE` | config file (default `config.txt`) |
| `-s N`, `-t N` | servers and clock cycles (otherwise you are prompted; Enter keeps the config value) |
| `-v` / `-q` | verbose / quiet log |
| `--switch` / `--single` | bonus Switch mode / one balancer |
| `-l FILE` | log file path |
| `--no-color` | plain console output |
| `-y` | never prompt |

## Console colors

Blue: server added. Yellow: server removed. Red: firewall block. Magenta: traffic burst. Cyan: periodic status. Green: finished.
The log file is always plain text.

## Settings

Everything is in `config.txt` (`key=value`). Firewall rules are in `blocked_ranges.txt` (ranges, CIDR or single IPs).

## Classes

Request, RequestQueue, WebServer, RequestGenerator, Firewall, LoadBalancer, Switch, Config, Logger. See the class list for details.
