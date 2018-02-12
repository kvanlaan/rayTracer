mkdir results
for file in *.ray; do 
	FNAME="${file%.*}"
	#echo $file
	echo $FNAME
	time ../ray/build-release/bin/ray $file results/"$FNAME".png
done

