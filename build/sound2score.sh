#!/bin/bash

if [[ $# -eq 0 ]] ; then
    echo echo "Usage: $0 <FILE>\n<FILE> is the path of a WAV audio file.\nExample: $0 input.wav"
    exit 1
fi

./audiotranscriber "$1" && musescore out.mid
