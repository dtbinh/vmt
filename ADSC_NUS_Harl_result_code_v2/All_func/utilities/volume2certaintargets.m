function [ targets ] = volume2certaintargets( imagepath, filter_length, model_ids, thresholds, ifdepth )
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here

if nargin < 5
    ifdepth = 0;
end

if nargin<4
    usetho = 0;
else
    usetho = 1;
end

CurrentTargets = [];
PastTargets = [];
matfilenames = dir([imagepath '/*.mat']);
framenums = 1+length(matfilenames);


for i = 2:framenums
%     disp(i);
    matfilename = [imagepath '/' matfilenames(i-1).name];
    %load the boxes: plot_boxes
    load(matfilename);
    
    boxes = [];
    if ~usetho
        for j = 1:length(model_ids)
            tempboxes = plot_boxes{mi(j)};
            if ~isempty(tempboxes)
                picked = nms(tempboxes,0.5);
                tempboxes = tempboxes(picked,:);
            end
            boxes = [boxes; tempboxes];
        end
    else
        for j = 1:length(model_ids)
            tempboxes = plot_boxes{model_ids(j)};
            if ~isempty(tempboxes)
                tempboxes = tempboxes(find(tempboxes(:,5)>thresholds(j)),:);
                picked = nms(tempboxes,0.5);
                tempboxes = tempboxes(picked,:);
            end
            boxes = [boxes; tempboxes];
        end
    end
    
    if (ifdepth == 1) && ~isempty(boxes)
%         s = regexp(imagepath,'\\','split');
%         foldername = s{end};
        foldername = imagepath(end-13:end-7);
        frameid = i-1;
        [ boxes ] = depthfilter( boxes, foldername, frameid );
    end
%     picked = nms(boxes,0.5);
%     boxes = boxes(picked,:);
%     [ boxes ] = depthfilter2_func( boxes, depthimage );
    CurrentTargets = boxes2targets( CurrentTargets,boxes,i );
    
    if ~isempty(CurrentTargets)
        del = [];
        for j = 1:length(CurrentTargets)
            if i - CurrentTargets{j}.data(end,5) > 5
                PastTargets{end + 1} = CurrentTargets{j};
                del = [del j];
            end
        end
        CurrentTargets(del) = [];
        
    end
end
%%%

alltargets = [CurrentTargets PastTargets];
counts = [];
for i = 1:length(alltargets)
if size(alltargets{i}.data,1) >filter_length
counts = [counts i];
end
end
% length(counts)

%match the targets
targets = alltargets(counts);
% targets{:}
[ targets ] = targetsinterp( targets );
end

