function [ fragments ] = search_static_fragments( data_path )
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here
fragments = [];
D = dir([data_path '\*.jpg']);

for i = 1 : length(D)-1
    frameA = imread([data_path '\' D(i).name]);
    frameB = imread([data_path '\' D(i+1).name]);
    oratios(i) = CompareTwoFrames( frameA, frameB );
end


oratios = medfilt1(oratios, 3);
% plot(forations)



indis = oratios<0.5;
if ~isempty(indis)
    % the part that less than 0.5 is the static part
    % we need find the fragment of static part
    indis = [0 indis 0];
    indis2 = indis(2:end) - indis(1:end-1);
    
    starts = find(indis2 == 1);
    ends = find(indis2==-1);
    
    fragments = [starts' ends'];
end

if size(fragments,1)>1
    close_fragments_indis = find((starts(2:end) - ends(1:end-1))<5);

    if ~isempty(close_fragments_indis)
        merge_ids = [];
        for i = 1:length(close_fragments_indis)
            fai = close_fragments_indis(i);
            fbi = close_fragments_indis(i)+1;
            temp_oratios = [];

            ablength = min( fragments(fai,2)-fragments(fai,1), fragments(fbi,2) - fragments(fbi,1));
            if ablength >100
                ablength =100;
            end
            for toi = 1:ablength
                frameA = imread([data_path '\' D(fragments(fai,1) +toi-1).name]);
                frameB = imread([data_path '\' D(fragments(fbi,1) +toi-1).name]);
                temp_oratios(toi) = CompareTwoFrames( frameA, frameB);
            end
            if (sum(temp_oratios<0.5)/ablength)>0.5
                merge_ids = [merge_ids; fai];
            end
        end
        if ~isempty(merge_ids)
           merge_ids = sort(merge_ids, 'descend'); 
           for mi = 1:length(merge_ids)
               fragments(merge_ids(mi),2) = fragments(merge_ids(mi)+1,2);
               fragments(merge_ids(mi)+1,:) = [];
           end
        end
    end     
end

end

