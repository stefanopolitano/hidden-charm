JSONCONFIG=dpl-config_hidden_charm_pp.json
COMMONCONFIGS="-b --shm-segment-size 62000000000 --readers 10 --io-threads 10 --spawners 10 --pipeline hf-data-creator-hidden-charm-reduced:40 --configuration json://$JSONCONFIG"
#o2-analysis-multcenttable $COMMONCONFIGS |\
o2-analysis-trackselection $COMMONCONFIGS |\
o2-analysis-event-selection-service $COMMONCONFIGS |\
o2-analysis-propagationservice $COMMONCONFIGS |\
o2-analysis-pid-tpc-service $COMMONCONFIGS |\
o2-analysis-track-to-collision-associator $COMMONCONFIGS |\
o2-analysis-pid-tof-base $COMMONCONFIGS |\
o2-analysis-pid-tof-full $COMMONCONFIGS |\
o2-analysis-tracks-extra-v002-converter $COMMONCONFIGS |\
o2-analysis-hf-data-creator-hidden-charm-reduced $COMMONCONFIGS |\
o2-analysis-hf-task-hidden-charm $COMMONCONFIGS --aod-file @input_data.txt --aod-writer-json OutputDirector.json
#o2-analysis-hf-track-index-skim-creator $COMMONCONFIGS --aod-file @input_data.txt --aod-writer-json OutputDirector.json