#!/bin/sh

# Simple passthrough wrapper so commands like `pm gh ...` behave as requested
exec "$@"
