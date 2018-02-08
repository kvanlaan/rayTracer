mkdir results
for file in *.ray; do 
	FNAME="${file%.*}"
	echo $file
	echo $FNAME
	../ray/build/bin/ray $file results/"$FNAME".png
done

