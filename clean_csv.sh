#!/bin/bash
# Usage: ./clean_csv.sh data.csv [output.csv]
# Strips pyterm noise and produces a clean CSV file.

INPUT="${1:?Usage: $0 <input> [output]}"
OUTPUT="${2:-${INPUT%.csv}_clean.csv}"

grep -E '# (node_id|[A-Z],[0-9]+,-?[0-9]+,[0-9]+)' "$INPUT" \
    | sed 's/^.*# //' \
    > "$OUTPUT"

echo "Wrote $(wc -l < "$OUTPUT") lines to $OUTPUT"
